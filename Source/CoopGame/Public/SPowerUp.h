// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPowerUp.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class COOPGAME_API ASPowerUp : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASPowerUp();

	void ActivatePowerup(AActor* ActiveFor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnActivated(AActor* ActiveFor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnPowerupTicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnExpired();

protected:


	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UPointLightComponent* LightComp;

	/* time between powerup ticks */
	UPROPERTY(EditDefaultsOnly, Category = "Powerups")
	float PowerupInterval;
	
	/*Total times we apply the powerup effect*/
	UPROPERTY(EditDefaultsOnly, Category = "Powerups")
	int32 TotalNrOfTicks;
	
	FTimerHandle TH_PowerUpTick;
	
	/*Total number of ticks apply*/
	int32 TickProcessed;

	UFUNCTION()
	void OnTickPowerup();

	UPROPERTY(ReplicatedUsing = OnRep_PowerupActive)
	bool bIsPowerupActive;

	UFUNCTION()
	void OnRep_PowerupActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnPowerupStateChanged(bool bNewIsActive);
};
