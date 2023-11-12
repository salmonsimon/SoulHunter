// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "CharacterType.h"
#include "BaseCharacter.h"
#include "Interfaces/PickupInterface.h"

#include "PlayerCharacter.generated.h"

#pragma region Forward Declarations

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UPlayerOverlay;
class AItem;
class UAnimMontage;
class ASoul;
class ATreasure;

#pragma endregion

UCLASS()
class CPPGAMEDEVCOURSE_API APlayerCharacter : public ABaseCharacter, public IPickupInterface
{
	GENERATED_BODY()

public:

#pragma region Main

	APlayerCharacter();
	virtual void Tick(float DeltaTime) override;
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;

	virtual void GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter) override;
	virtual void SetOverlappingItem(AItem* Item) override;
	virtual void AddSouls(ASoul* Soul) override;
	virtual void AddGold(ATreasure* Treasure) override;

#pragma endregion

protected:

#pragma region Main

	virtual void BeginPlay() override;
	virtual void Death(const FVector& ImpactPoint) override;
	void DropWeapon();
	UFUNCTION(BlueprintCallable) void BackToUnoccupiedState();

#pragma endregion

#pragma region Input

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void InteractKeyPressed();

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* MappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MovementAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* DodgeAction;

#pragma endregion

#pragma region Combat

	virtual void Attack() override;
	virtual bool CanAttack() override;
	UFUNCTION(BlueprintCallable) void Disarm();
	UFUNCTION(BlueprintCallable) void Arm();
	bool CanDisarm();
	bool CanArm();
	void PlayArmDisarmMontage(const FName& SectionName);
	void Dodge();
	bool HasEnoughStamina();

#pragma endregion

private:

#pragma region Main

	ECharacterState CharacterState = ECharacterState::ECS_Unequipped;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	EActionState ActionState = EActionState::EAS_Unoccupied;

#pragma endregion

#pragma region Main Components

	void InitializePlayerOverlay(APlayerController* PlayerController);

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* ViewCamera;

	UPROPERTY(VisibleInstanceOnly)
	AItem* OverlappingItem;

	UPROPERTY()
	UPlayerOverlay* PlayerOverlay;

#pragma endregion

#pragma region Montages

	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* ArmDisarmMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* DodgeMontage;

#pragma endregion

public:

#pragma region Getters/Setters

	UFUNCTION(BlueprintCallable) FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }

	FORCEINLINE bool IsUnoccupied() const { return ActionState == EActionState::EAS_Unoccupied; }

#pragma endregion
	
};
