// Fill out your copyright notice in the Description page of Project Settings.

#include "SPickupActor.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "SPowerUp.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ASPickupActor::ASPickupActor()
{
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(75.f);
	
	RootComponent = SphereComp;

	DecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	DecalComp->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	DecalComp->DecalSize = FVector(64.f, 75.f, 75.f);
	DecalComp->SetupAttachment(RootComponent);

	RespawnTime = 10.f;

	SetReplicates(true);
}

void ASPickupActor::NotifyActorBeginOverlap(AActor * OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	if (Role == ROLE_Authority && PowerUpInst)
	{
		PowerUpInst->ActivatePowerup(OtherActor);

		PowerUpInst = nullptr;

		GetWorldTimerManager().SetTimer(TH_Respawn, this, &ASPickupActor::Respawn, RespawnTime);
	}
}

// Called when the game starts or when spawned
void ASPickupActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (Role == ROLE_Authority)
	{
		Respawn();
	}
}

void ASPickupActor::Respawn()
{
	if (PowerupClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("PowerUpClass is nullptr in %s. Please update your blueprint"), *GetName());
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PowerUpInst = GetWorld()->SpawnActor<ASPowerUp>(PowerupClass, GetTransform(), SpawnParams);

}


