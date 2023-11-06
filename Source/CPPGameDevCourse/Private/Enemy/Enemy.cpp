// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "Components\SkeletalMeshComponent.h"
#include "Components\CapsuleComponent.h"
#include "CPPGameDevCourse\DebugMacros.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AttributeComponent.h"
#include "HUD/HealthBarComponent.h"
#include "Characters/CharacterType.h"

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

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));

	HealthBarWidget = CreateDefaultSubobject<UHealthBarComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(GetRootComponent());
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

void AEnemy::PlayHitReactMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HitReactionMontage)
	{
		AnimInstance->Montage_Play(HitReactionMontage);
		AnimInstance->Montage_JumpToSection(SectionName, HitReactionMontage);
	}
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CombatTarget)
	{
		const double CombatTargetDistance = (CombatTarget->GetActorLocation() - GetActorLocation()).Size();

		if (CombatTargetDistance > CombatRadius)
		{
			CombatTarget = nullptr;

			if (HealthBarWidget)
				HealthBarWidget->SetVisibility(false);
		}
	}
}

// Called to bind functionality to input
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

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Attributes && HealthBarWidget)
	{
		Attributes->ReceiveDamage(DamageAmount);
		HealthBarWidget->SetHealthPercent(Attributes->GetHealthPercent());
	}

	CombatTarget = EventInstigator->GetPawn();

	return DamageAmount;
}

