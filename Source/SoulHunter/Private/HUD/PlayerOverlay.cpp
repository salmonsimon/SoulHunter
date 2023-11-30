// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/PlayerOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerOverlay::SetHealthBarPercent(float Percent)
{
	if (HealthProgressBar)
		HealthProgressBar->SetPercent(Percent);
}

void UPlayerOverlay::SetStaminaBarPercent(float Percent)
{
	if (StaminaProgressBar)
		StaminaProgressBar->SetPercent(Percent);
}

void UPlayerOverlay::SetGoldCountText(int32 Gold)
{
	if (GoldCountText)
		GoldCountText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Gold)));
}

void UPlayerOverlay::SetSoulsCountText(int32 Souls)
{
	if (SoulsCountText)
		SoulsCountText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Souls)));
}
