// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/Enemy.h"

#include "AIController.h"

#include "Components\SkeletalMeshComponent.h"
#include "Components\CapsuleComponent.h"
#include "Components/AttributeComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/HealthBarComponent.h"

#include "Characters/CharacterType.h"

#include "CPPGameDevCourse\DebugMacros.h"

#include "Animation/AnimMontage.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetGenerateOverlapEvents(true);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));

	HealthBarWidget = CreateDefaultSubobject<UHealthBarComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(GetRootComponent());

	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("Pawn Sensing"));
	PawnSensing->SightRadius = 2000.f;
	PawnSensing->SetPeripheralVisionAngle(45.f);
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
		HealthBarWidget->SetHealthPercent(1.f);
	}

	EnemyController = Cast<AAIController>(GetController());

	if (EnemyController && PatrolTarget)
		MoveToTarget(PatrolTarget);

	if (PawnSensing)
		PawnSensing->OnSeePawn.AddDynamic(this, &AEnemy::PawnSeen);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (EnemyState > EEnemyState::EES_Patrolling)
		CheckCombatTarget();
	else
		CheckPatrolTarget();
}

void AEnemy::MoveToTarget(AActor* Target)
{
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(Target);
	MoveRequest.SetAcceptanceRadius(15.f);

	EnemyController->MoveTo(MoveRequest);
}

bool AEnemy::InTargetRange(AActor* Target, double Radius)
{
	if (Target == nullptr) return false;

	const double TargetDistance = (Target->GetActorLocation() - GetActorLocation()).Size();
	return TargetDistance <= Radius;
}

void AEnemy::CheckCombatTarget()
{
	if (!InTargetRange(CombatTarget, CombatRadius))
	{
		CombatTarget = nullptr;

		if (HealthBarWidget)
			HealthBarWidget->SetVisibility(false);

		EnemyState = EEnemyState::EES_Patrolling;
		MoveToTarget(PatrolTarget);

		GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;
	}
	else if (!InTargetRange(CombatTarget, AttackRadius) &&
		EnemyState != EEnemyState::EES_Chasing)
	{
		EnemyState = EEnemyState::EES_Chasing;

		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
		MoveToTarget(CombatTarget);
	}
	else if (InTargetRange(CombatTarget, AttackRadius) &&
		EnemyState != EEnemyState::EES_Attacking)
	{
		EnemyState = EEnemyState::EES_Attacking;

		//TODO: ADD ATTACK MONTAGE
	}
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

void AEnemy::PawnSeen(APawn* SeenPawn)
{
	if (EnemyState == EEnemyState::EES_Chasing) return;

	if (SeenPawn->ActorHasTag(FName("Player")))
	{
		GetWorldTimerManager().ClearTimer(PatrolTimer);
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
		CombatTarget = SeenPawn;

		if (EnemyState != EEnemyState::EES_Attacking)
		{
			EnemyState = EEnemyState::EES_Chasing;
			MoveToTarget(CombatTarget);
		}
	}
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::GetHit_Implementation(const FVector& ImpactPoint)
{
	if (HealthBarWidget)
		HealthBarWidget->SetVisibility(true);

	if (Attributes && Attributes->IsAlive())
		DirectionalHitReact(ImpactPoint);
	else
		Death();

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
	}

	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, ImpactPoint);
	}
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Attributes && HealthBarWidget)
	{
		Attributes->ReceiveDamage(DamageAmount);
		HealthBarWidget->SetHealthPercent(Attributes->GetHealthPercent());
	}

	CombatTarget = EventInstigator->GetPawn();
	EnemyState = EEnemyState::EES_Chasing;
	GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	MoveToTarget(CombatTarget);

	return DamageAmount;
}

void AEnemy::DirectionalHitReact(const FVector& ImpactPoint)
{
	const FVector Forward = GetActorForwardVector();

	const FVector ImpactLowered(ImpactPoint.X, ImpactPoint.Y, GetActorLocation().Z);
	const FVector ToHit = (ImpactLowered - GetActorLocation()).GetSafeNormal();

	const double CosTheta = FVector::DotProduct(Forward, ToHit);
	double HitAngle = FMath::RadiansToDegrees(FMath::Acos(CosTheta));

	const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit);
	if (CrossProduct.Z < 0)
		HitAngle *= -1.f;

	FName SectionName = FName("ReactFromBack");

	if (HitAngle >= -45.f && HitAngle < 45.f)
		SectionName = FName("ReactFromFront");
	else if (HitAngle >= -135.f && HitAngle < -45.f)
		SectionName = FName("ReactFromLeft");
	else if (HitAngle >= 45.f && HitAngle < 135.f)
		SectionName = FName("ReactFromRight");

	PlayHitReactMontage(SectionName);
}

void AEnemy::PlayHitReactMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HitReactionMontage)
	{
		AnimInstance->Montage_Play(HitReactionMontage);
		AnimInstance->Montage_JumpToSection(SectionName, HitReactionMontage);
	}
}

void AEnemy::Death()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		const int32 TotalSections = DeathMontage->GetNumSections();
		int32 DeathSelection = FMath::RandRange(0, TotalSections - 1);

		const FName SectionName = DeathMontage->GetSectionName(DeathSelection);
		DeathPose = (EDeathPose)(DeathSelection + 1);

		AnimInstance->Montage_Play(DeathMontage);
		AnimInstance->Montage_JumpToSection(SectionName, DeathMontage);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (HealthBarWidget)
		HealthBarWidget->SetVisibility(false);

	SetLifeSpan(30.f);
}