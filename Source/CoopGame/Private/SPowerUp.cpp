// Fill out your copyright notice in the Description page of Project Settings.

#include "SPowerUp.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASPowerUp::ASPowerUp()
{
	
	
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = MeshComp;
	
	LightComp = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightComp"));
	LightComp->SetCastShadows(false);
	LightComp->SetAttenuationRadius(200.f);
	LightComp->SetupAttachment(MeshComp);


	PowerupInterval = 0.f;
	TotalNrOfTicks = 0.f;

	bIsPowerupActive = false;

	SetReplicates(true);

}

void ASPowerUp::ActivatePowerup(AActor* ActiveFor)
{
	OnActivated(ActiveFor);

	bIsPowerupActive = true;
	OnRep_PowerupActive();

	if (PowerupInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(TH_PowerUpTick, this, &ASPowerUp::OnTickPowerup, PowerupInterval, true);
	}
	else
	{
		OnTickPowerup();

	}

}



void ASPowerUp::OnTickPowerup()
{
	TickProcessed++;

	OnPowerupTicked();

	if (TickProcessed >= TotalNrOfTicks)
	{
		OnExpired();

		bIsPowerupActive = false;
		OnRep_PowerupActive();

		GetWorldTimerManager().ClearTimer(TH_PowerUpTick);

	}

}

void ASPowerUp::OnRep_PowerupActive()
{

	OnPowerupStateChanged(bIsPowerupActive);
}


void ASPowerUp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPowerUp, bIsPowerupActive);

}