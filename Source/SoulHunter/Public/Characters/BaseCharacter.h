// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/HitInterface.h"
#include "GameFramework/Character.h"
#include "Engine/SkeletalMesh.h"

#include "BaseCharacter.generated.h"

#pragma region Forward Declarations

class AWeapon;
class UAnimMontage;
class UAttributeComponent;

#pragma endregion

UCLASS()
class SOULHUNTER_API ABaseCharacter : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:

#pragma region Main

	ABaseCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// IHitInterface
	virtual void GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter) override;

#pragma endregion

#pragma region Combat

	UFUNCTION(BlueprintCallable)
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

#pragma endregion

protected:

#pragma region Main

	virtual void BeginPlay() override;
	virtual void Death(const FVector& ImpactPoint);
	void StartRagdoll(const FVector& ImpactPoint, const float& ImpulseStrenght);
	bool IsAlive();

	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	FName RagdollBaseBone = FName("pelvis");

#pragma endregion

#pragma region Main Components

	UPROPERTY(VisibleAnywhere)
	UAttributeComponent* Attributes;

	UPROPERTY(VisibleAnywhere, Category = Weapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(EditAnywhere, Category = Weapon, meta = (GetOptions = "GetSocketNames"))
	FName WeaponSocket;

	UFUNCTION()
	TArray<FName> GetSocketNames() const
	{
		TArray<FName> SocketNames;

		SocketNames.Append(GetMesh()->GetAllSocketNames());

		return SocketNames;
	}

#pragma endregion

#pragma region Combat

	virtual void Attack() PURE_VIRTUAL(ABaseCharacter::Attack);
	virtual bool CanAttack() PURE_VIRTUAL(ABaseCharacter::Attack, return false;);

	void DirectionalHitReact(const FVector& ImpactPoint);
	void PlayHitReactMontage(const FName& SectionName);
	void PlayHitSound(const FVector& ImpactPoint);
	void SpawnHitParticles(const FVector& ImpactPoint);

	void StopAttackMontage();

	UFUNCTION(BlueprintCallable)
	FVector GetTranslationWarpTarget();

	UFUNCTION(BlueprintCallable)
	FVector GetRotationWarpTarget();

	UPROPERTY(BlueprintReadWrite, Category = Combat)
	AActor* CombatTarget;

	UPROPERTY(EditAnywhere, Category = Combat)
	double WarpTargetDistance = 10.f;

	int32 LastSelectedAttackMontageSection = -1;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UAnimMontage* HitReactionMontage;

#pragma endregion

#pragma region Montages

	int32 PlayMontageRandomSection(UAnimMontage* AnimationMontage);
	int32 PlayMontageRandomSection(UAnimMontage* AnimationMontage, int32& LastSelectedIndex);

#pragma endregion

private:

#pragma region Combat Effect Variables

	UPROPERTY(EditAnywhere, Category = "Combat")
	USoundBase* HitSound;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UParticleSystem* HitParticles;

#pragma endregion
	
};
