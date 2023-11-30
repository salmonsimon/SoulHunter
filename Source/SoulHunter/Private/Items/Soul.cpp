// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Soul.h"
#include "Interfaces/PickupInterface.h"
#include "Kismet/KismetSystemLibrary.h"


void ASoul::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bFinishedDrifting)
	{
		const FVector Location = GetActorLocation();

		ElapsedDriftingTime += DeltaTime;
		float DriftingPercent = ElapsedDriftingTime / DriftingTime;
		
		SetActorLocation(FMath::Lerp(Location, DesiredLocation, DriftingPercent));

		if (DriftingPercent >= 1.f)
			bFinishedDrifting = true;
	}
	
}

void ASoul::BeginPlay()
{
	Super::BeginPlay();

	FVector LineTraceStart = GetActorLocation();
	FVector LineTraceEnd = LineTraceStart - FVector(0.f, 0.f, 2000.f);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery1);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	FHitResult HitResult;

	UKismetSystemLibrary::LineTraceSingleForObjects(
		this, 
		LineTraceStart, 
		LineTraceEnd, 
		ObjectTypes, 
		false,
		ActorsToIgnore, 
		bShowLineTraceDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		HitResult, 
		true
	);

	DesiredLocation = HitResult.ImpactPoint + FVector(0.f, 0.f, 75.f);
}

void ASoul::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
	if (PickupInterface)
	{
		PickupInterface->AddSouls(this);

		SpawnPickupEffect();
		SpawnPickupSound();

		Destroy();
	}
}
