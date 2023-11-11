// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/AttributeComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#pragma region Main

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

float ABaseCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Attributes)
		Attributes->ReceiveDamage(DamageAmount);

	return DamageAmount;
}

void ABaseCharacter::GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter)
{
	if (IsAlive() && Hitter)
		DirectionalHitReact(Hitter->GetActorLocation());
	else
		Death();

	PlayHitSound(ImpactPoint);
	SpawnHitParticles(ImpactPoint);

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ABaseCharacter::IsAlive()
{
	return Attributes && Attributes->IsAlive();
}

#pragma endregion

#pragma region Combat

void ABaseCharacter::DirectionalHitReact(const FVector& ImpactPoint)
{
	if (HitReactionMontage == nullptr)
		return;

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

void ABaseCharacter::PlayHitReactMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HitReactionMontage)
	{
		AnimInstance->Montage_Play(HitReactionMontage);
		AnimInstance->Montage_JumpToSection(SectionName, HitReactionMontage);
	}
}

void ABaseCharacter::PlayHitSound(const FVector& ImpactPoint)
{
	if (HitSound)
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
}

void ABaseCharacter::SpawnHitParticles(const FVector& ImpactPoint)
{
	if (HitParticles)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, ImpactPoint);
}

void ABaseCharacter::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	if (EquippedWeapon && EquippedWeapon->GetWeaponCollisionBox())
	{
		EquippedWeapon->ResetHitIgnoreActors();
		EquippedWeapon->GetWeaponCollisionBox()->SetCollisionEnabled(CollisionEnabled);
	}
}

#pragma endregion

#pragma region Montages

int32 ABaseCharacter::PlayMontageRandomSection(UAnimMontage* AnimationMontage)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && AnimationMontage)
	{
		const int32 TotalSections = AnimationMontage->GetNumSections();
		int32 SectionSelection = FMath::RandRange(0, TotalSections - 1);

		const FName SectionName = AnimationMontage->GetSectionName(SectionSelection);

		AnimInstance->Montage_Play(AnimationMontage);
		AnimInstance->Montage_JumpToSection(SectionName, AnimationMontage);

		return SectionSelection;
	}
	else
		return -1;
}

int32 ABaseCharacter::PlayMontageRandomSection(UAnimMontage* AnimationMontage, int32& LastSelectedIndex)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && AnimationMontage)
	{
		const int32 TotalSections = AnimationMontage->GetNumSections();
		int32 SectionSelection = FMath::RandRange(0, TotalSections - 1);

		while (SectionSelection == LastSelectedIndex)
			SectionSelection = FMath::RandRange(0, TotalSections - 1);

		LastSelectedIndex = SectionSelection;

		const FName SectionName = AnimationMontage->GetSectionName(SectionSelection);

		AnimInstance->Montage_Play(AnimationMontage);
		AnimInstance->Montage_JumpToSection(SectionName, AnimationMontage);

		return SectionSelection;
	}

	return -1;
}

void ABaseCharacter::StopAttackMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance)
		AnimInstance->Montage_Stop(.25f, AttackMontage);
}

#pragma endregion

FVector ABaseCharacter::GetTranslationWarpTarget()
{
	if (CombatTarget == nullptr) return FVector();

	const FVector CombatTargetLocation = CombatTarget->GetActorLocation();
	const FVector Location = GetActorLocation();

	const FVector TargetToThisActor = Location - CombatTargetLocation;

	return CombatTargetLocation + TargetToThisActor.GetSafeNormal() * WarpTargetDistance;
}

FVector ABaseCharacter::GetRotationWarpTarget()
{
	if (CombatTarget)
		return CombatTarget->GetActorLocation();

	return FVector();
}
