// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Characters/BaseCharacter.h"
#include "Characters/CharacterType.h"

#include "Enemy.generated.h"

#pragma region Forward Declarations

class UAnimMontage;
class UAttributeComponent;
class UHealthBarComponent;
class UPawnSensingComponent;

#pragma endregion

UCLASS()
class CPPGAMEDEVCOURSE_API AEnemy : public ABaseCharacter
{
	GENERATED_BODY()

public:

#pragma region Main

	AEnemy();

	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Destroyed() override;

	// IHitInterface
	virtual void GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter) override;

#pragma endregion

protected:

#pragma region Main

	virtual void BeginPlay() override;

	virtual void Death(const FVector& ImpactPoint) override;

	void SpawnSoul();

	UPROPERTY(BlueprintReadOnly)
	EEnemyDeathPose DeathPose;

	UPROPERTY(EditAnywhere)
	float DeathLifeSpan = 30.f;

#pragma endregion

#pragma region AI Behavior - Main

	void MoveToTarget(AActor* PatrolTarget);
	bool InTargetRange(AActor* Target, double Radius);
	UFUNCTION() void PawnSeen(APawn* SeenPawn);

	UPROPERTY(BlueprintReadOnly)
	EEnemyState EnemyState = EEnemyState::EES_Patrolling;

#pragma endregion

#pragma region AI Behavior - Combat

	virtual void Attack() override;
	virtual bool CanAttack() override;
	UFUNCTION(BlueprintCallable) void AttackEnd();

#pragma endregion

private:

#pragma region Main Components

	void SpawnDefaultWeapon();

	UPROPERTY()
	class AAIController* EnemyController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AWeapon> WeaponClass;

	UPROPERTY(VisibleAnywhere)
	UPawnSensingComponent* PawnSensing;

	UPROPERTY(EditAnywhere)
	float PawnSightRadius = 2000.f;

	UPROPERTY (EditAnywhere)
	float PawnPeripheralVisionAngle = 45.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	TSubclassOf<class ASoul> SoulClass;

#pragma endregion

#pragma region AI Behavior - Patrol

	void StartPatrolling();
	void CheckPatrolTarget();
	void ChooseNewPatrolTarget();
	void PatrolTimerFinished();
	void ClearPatrolTimer();

	UPROPERTY(EditAnywhere, Category = Combat) 
	float PatrollingSpeed = 125.f;

	UPROPERTY(EditInstanceOnly, Category = "AI Navigation")
	AActor* PatrolTarget;

	UPROPERTY(EditInstanceOnly, Category = "AI Navigation")
	TArray<AActor*> PatrolTargets;

	UPROPERTY(EditAnywhere)
	double PatrolRadius = 200.f;

	FTimerHandle PatrolTimer;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float PatrolWaitinTimeMin = 4.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float PatrolWaitingTimeMax = 10.f;

#pragma endregion

#pragma region AI Behavior - Chase

	void LoseInterest();
	void StartChasing();

	UPROPERTY(EditAnywhere, Category = Combat)
	float ChasingSpeed = 350.f;

#pragma endregion

#pragma region AI Behavior - Combat

	void CheckCombatTarget();
	void StartAttackTimer();
	void ClearAttackTimer();
	bool IsOutsideCombatRadius();
	bool IsOutsideAttackRadius();
	bool IsInsideAttackRadius();

	UPROPERTY(EditAnywhere)
	float CombatRadius = 500.f;

	UPROPERTY(EditAnywhere)
	float AttackRadius = 135.f;

	FTimerHandle AttackTimer;

	UPROPERTY(EditAnywhere, Category = Combat)
	float AttackWaitingTimeMin = .5f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float AttackWaitingTimeMax = 1.f;

#pragma endregion

#pragma region AI Behavior - State Booleans
	bool IsChasing();
	bool IsAttacking();
	bool IsEngaged();
	bool IsDead();
#pragma endregion

#pragma region Health Widget

	void UpdateHealthPercent();
	void ShowHealthBar(bool Show);

	UPROPERTY(VisibleAnywhere) 
	UHealthBarComponent* HealthBarWidget;

#pragma endregion

};
