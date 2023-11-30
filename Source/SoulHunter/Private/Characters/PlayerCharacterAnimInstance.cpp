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
		
		UpdateShouldMove();
		UpdateDirectionAngle();
	}
}

void UPlayerCharacterAnimInstance::UpdateShouldMove()
{
	ShouldMove = GroundSpeed > 3.0f &&
		         !PlayerMovementComponent->GetCurrentAcceleration().IsZero();
}

void UPlayerCharacterAnimInstance::UpdateDirectionAngle()
{
	float DirectionAngle_LastTick = DirectionAngle;

	FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	float LocalDirection = CalculateDirection(Velocity, ActorRotation);

	if (FMath::IsNearlyEqual(FMath::Abs(LocalDirection), BACKWARD_DIRECTION_CONSTANT, 1.f)) 
		DirectionAngle = DirectionAngle_LastTick < 0 ? -BACKWARD_DIRECTION_CONSTANT : BACKWARD_DIRECTION_CONSTANT;
	else
		DirectionAngle = LocalDirection;
}
