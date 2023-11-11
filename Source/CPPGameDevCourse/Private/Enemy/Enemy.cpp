// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/Enemy.h"
#include "AIController.h"
#include "Components\SkeletalMeshComponent.h"
#include "Components\CapsuleComponent.h"
#include "Components/AttributeComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/HealthBarComponent.h"
#include "Items/Weapons/Weapon.h"

#pragma region Main

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	HealthBarWidget = CreateDefaultSubobject<UHealthBarComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(GetRootComponent());

	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("Pawn Sensing"));
	PawnSensing->SightRadius = PawnSightRadius;
	PawnSensing->SetPeripheralVisionAngle(PawnPeripheralVisionAngle);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead()) return;

	if (EnemyState == EEnemyState::EES_Patrolling)
		CheckPatrolTarget();
	else
		CheckCombatTarget();
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ABaseCharacter::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	UpdateHealthPercent();

	CombatTarget = EventInstigator->GetPawn();

	if (IsInsideAttackRadius())
		EnemyState = EEnemyState::EES_Attacking;
	else if (IsOutsideAttackRadius())
		StartChasing();

	return DamageAmount;
}

void AEnemy::GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter)
{
	if (EnemyState == EEnemyState::EES_Dead) return;

	ShowHealthBar(true);

	ABaseCharacter::GetHit_Implementation(ImpactPoint, Hitter);

	ClearPatrolTimer();
	ClearAttackTimer();

	StopAttackMontage();
}

void AEnemy::Destroyed()
{
	if (EquippedWeapon)
		EquippedWeapon->Destroy();
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (PawnSensing)
		PawnSensing->OnSeePawn.AddDynamic(this, &AEnemy::PawnSeen);

	ShowHealthBar(false);
	SpawnDefaultWeapon();

	EnemyController = Cast<AAIController>(GetController());

	if (EnemyController && PatrolTarget)
	{
		FTimerHandle UnusedHandle;
		GetWorldTimerManager().SetTimer(UnusedHandle, this, &AEnemy::StartPatrolling, .1f, false);
	}

	Tags.Add(FName("Enemy"));
}

void AEnemy::Death()
{
	EnemyState = EEnemyState::EES_Dead;

	ClearAttackTimer();
	ClearPatrolTimer();

	int32 DeathSectionSelected = PlayMontageRandomSection(DeathMontage);
	DeathPose = (EEnemyDeathPose)DeathSectionSelected;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ShowHealthBar(false);

	SetLifeSpan(DeathLifeSpan);

	GetCharacterMovement()->bOrientRotationToMovement = false;

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
}

#pragma endregion

#pragma region Main Components

void AEnemy::SpawnDefaultWeapon()
{
	UWorld* World = GetWorld();

	if (World && WeaponClass)
	{
		AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(WeaponClass);
		DefaultWeapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
		EquippedWeapon = DefaultWeapon;
	}
}

#pragma endregion

#pragma region AI Behavior - Main

void AEnemy::MoveToTarget(AActor* Target)
{
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(Target);
	MoveRequest.SetAcceptanceRadius(40.f);

	EnemyController->MoveTo(MoveRequest);
}

bool AEnemy::InTargetRange(AActor* Target, double Radius)
{
	if (Target == nullptr) return false;

	const double TargetDistance = (Target->GetActorLocation() - GetActorLocation()).Size();
	return TargetDistance <= Radius;
}

void AEnemy::PawnSeen(APawn* SeenPawn)
{
	const bool bShouldChaseTarget =
		EnemyState == EEnemyState::EES_Patrolling &&
		SeenPawn->ActorHasTag(FName("EngageableTarget"));

	if (bShouldChaseTarget)
	{
		CombatTarget = SeenPawn;

		ClearPatrolTimer();

		StartChasing();
	}
}

#pragma endregion

#pragma region AI Behavior - Patrolling

void AEnemy::StartPatrolling()
{
	EnemyState = EEnemyState::EES_Patrolling;

	GetCharacterMovement()->MaxWalkSpeed = PatrollingSpeed;

	MoveToTarget(PatrolTarget);
}

void AEnemy::CheckPatrolTarget()
{
	if (EnemyController && InTargetRange(PatrolTarget, PatrolRadius))
	{
		ChooseNewPatrolTarget();

		float RandomWaitingTime = FMath::RandRange(PatrolWaitinTimeMin, PatrolWaitingTimeMax);

		GetWorldTimerManager().SetTimer(PatrolTimer, this, &AEnemy::PatrolTimerFinished, RandomWaitingTime);
	}
}

void AEnemy::ChooseNewPatrolTarget()
{
	const int32 TotalPatrolTargets = PatrolTargets.Num();
	int32 TargetSelectedIndex = FMath::RandRange(0, TotalPatrolTargets - 1);

	AActor* TargetSelected = PatrolTargets[TargetSelectedIndex];

	while (TargetSelected == PatrolTarget)
	{
		TargetSelectedIndex = FMath::RandRange(0, TotalPatrolTargets - 1);
		TargetSelected = PatrolTargets[TargetSelectedIndex];
	}

	PatrolTarget = TargetSelected;
}

void AEnemy::PatrolTimerFinished()
{
	MoveToTarget(PatrolTarget);
}

void AEnemy::ClearPatrolTimer()
{
	GetWorldTimerManager().ClearTimer(PatrolTimer);
}

#pragma endregion

#pragma region AI Behavior - Chase

void AEnemy::LoseInterest()
{
	CombatTarget = nullptr;

	ShowHealthBar(false);
}

void AEnemy::StartChasing()
{
	EnemyState = EEnemyState::EES_Chasing;

	GetCharacterMovement()->MaxWalkSpeed = ChasingSpeed;

	MoveToTarget(CombatTarget);
}

#pragma endregion

#pragma region AI Behavior - Combat

void AEnemy::Attack()
{
	EnemyState = EEnemyState::EES_Engaged;
	PlayMontageRandomSection(AttackMontage, LastSelectedAttackMontageSection);
}

bool AEnemy::CanAttack()
{
	return !IsDead() && !IsAttacking() && IsInsideAttackRadius() && !IsEngaged();
}

void AEnemy::AttackEnd()
{
	EnemyState = EEnemyState::EES_NoState;
	CheckCombatTarget();
}

void AEnemy::CheckCombatTarget()
{
	if (IsOutsideCombatRadius())
	{
		ClearAttackTimer();
		LoseInterest();

		if (!IsEngaged())
			StartPatrolling();
	}
	else if (IsOutsideAttackRadius() && !IsChasing())
	{
		ClearAttackTimer();

		if (!IsEngaged())
			StartChasing();
	}
	else if (CanAttack())
	{
		StartAttackTimer();
	}
}

void AEnemy::StartAttackTimer()
{
	EnemyState = EEnemyState::EES_Attacking;

	const float AttackWaitingTime = FMath::RandRange(AttackWaitingTimeMin, AttackWaitingTimeMax);
	GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackWaitingTime);
}

void AEnemy::ClearAttackTimer()
{
	GetWorldTimerManager().ClearTimer(AttackTimer);
}

bool AEnemy::IsOutsideCombatRadius()
{
	return !InTargetRange(CombatTarget, CombatRadius);
}

bool AEnemy::IsOutsideAttackRadius()
{
	return !InTargetRange(CombatTarget, AttackRadius);
}

bool AEnemy::IsInsideAttackRadius()
{
	return InTargetRange(CombatTarget, AttackRadius);
}

#pragma endregion

#pragma region AI Behavior - State Booleans

bool AEnemy::IsChasing()
{
	return EnemyState == EEnemyState::EES_Chasing;
}

bool AEnemy::IsAttacking()
{
	return EnemyState == EEnemyState::EES_Attacking;
}

bool AEnemy::IsEngaged()
{
	return EnemyState == EEnemyState::EES_Engaged;
}

bool AEnemy::IsDead()
{
	return EnemyState == EEnemyState::EES_Dead;
}

#pragma endregion

#pragma region Health Widget

void AEnemy::ShowHealthBar(bool Show)
{
	if (HealthBarWidget)
		HealthBarWidget->SetVisibility(Show);
}

void AEnemy::UpdateHealthPercent()
{
	if (HealthBarWidget)
		HealthBarWidget->SetHealthPercent(Attributes->GetHealthPercent());
}

#pragma endregion
