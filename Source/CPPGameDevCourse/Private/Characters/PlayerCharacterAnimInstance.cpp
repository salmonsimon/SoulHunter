// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/PlayerCharacterAnimInstance.h"
#include "Characters\PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UPlayerCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
	if (PlayerCharacter)
	{
		PlayerMovementComponent = PlayerCharacter->GetCharacterMovement();
	}
}

void UPlayerCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (PlayerMovementComponent)
	{
		Velocity = PlayerMovementComponent->Velocity;
		GroundSpeed = UKismetMathLibrary::VSizeXY(Velocity);
		IsFalling = PlayerMovementComponent->IsFalling();
		CharacterState = PlayerCharacter->GetCharacterState();
	}
}
