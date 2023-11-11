// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Weapon.h"
#include "Characters\PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Components\SphereComponent.h"
#include "Components\BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces\HitInterface.h"
#include "NiagaraComponent.h"

AWeapon::AWeapon()
{
	ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Collision Box"));
	WeaponCollisionBox->SetupAttachment(GetRootComponent());

	WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponCollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	WeaponCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(GetRootComponent());

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(GetRootComponent());
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	WeaponCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnBoxOverlap);
}

void AWeapon::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	ItemState = EItemState::EIS_Equipped;

	SetOwner(NewOwner);
	SetInstigator(NewInstigator);

	AttackMeshToSocket(InParent, InSocketName);

	if (EquipSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), EquipSound, GetActorLocation());

	if (SphereCollider)
		SphereCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (EmbersEffect)
		EmbersEffect->Deactivate();
}

void AWeapon::AttackMeshToSocket(USceneComponent* InParent, const FName& InSocketName)
{
	ItemMesh->AttachToComponent(InParent, FAttachmentTransformRules::SnapToTargetIncludingScale, InSocketName);
}

void AWeapon::ResetHitIgnoreActors()
{
	HitIgnoreActors.Empty();
	HitIgnoreActors.Add(GetOwner());
}

void AWeapon::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ActorIsSameType(OtherActor))
		return;

	FHitResult BoxHitResult;
	BoxTrace(BoxHitResult);

	if (BoxHitResult.GetActor())
	{
		if (ActorIsSameType(BoxHitResult.GetActor()))
			return;

		UGameplayStatics::ApplyDamage(
			BoxHitResult.GetActor(),
			Damage,
			GetInstigator()->GetController(),
			this,
			UDamageType::StaticClass()
		);

		if (IHitInterface* HitInterface = Cast<IHitInterface>(BoxHitResult.GetActor()))
			HitInterface->Execute_GetHit(BoxHitResult.GetActor(), BoxHitResult.ImpactPoint, GetOwner());

		CreateFields(BoxHitResult.ImpactPoint);
	}
}

bool AWeapon::ActorIsSameType(AActor* OtherActor)
{
	return GetOwner()->ActorHasTag(FName("Enemy")) && OtherActor->ActorHasTag(FName("Enemy"));
}

void AWeapon::BoxTrace(FHitResult& BoxHitResult)
{
	const FVector Start = BoxTraceStart->GetComponentLocation();
	const FVector End = BoxTraceEnd->GetComponentLocation();

	UKismetSystemLibrary::BoxTraceSingle(
		this,
		Start,
		End,
		BoxTraceExtent,
		BoxTraceStart->GetComponentRotation(),
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		HitIgnoreActors,
		bShowBoxTraceDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		BoxHitResult,
		true
	);

	HitIgnoreActors.AddUnique(BoxHitResult.GetActor());
}
