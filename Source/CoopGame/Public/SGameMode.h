// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"


enum class EWaveState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilled, AActor*, DeadActor, AActor*, KillerActor, AController*, KillerController);

/**
 * 
 */
UCLASS()
class COOPGAME_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:

	ASGameMode();

	virtual void StartPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category = "GameMode")
	FOnActorKilled OnActorKilled;

	
protected:

	
	FTimerHandle TH_BotSpawner;

	FTimerHandle TH_NextWaveStart;

	int32 NrOfBotsToSpawn;

	int32 WaveCount;

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	float TimeBetweenWaves;

	// look to bp to spawn a single bot 
	UFUNCTION(BlueprintImplementableEvent, Category = "Gamemode")
	void SpawnNewBot();

	void SpawnBotTimerElapsed();
	
	//start spawning bots
	void StartWave();

	//end spawning bots 
	void EndWave();

	//set timer for the next start wave
	void PrepareForNextWave();

	void CheckWaveState();

	void CheckAnyPlayerAlive();

	void GameOver();

	void Respawn();

	void SetWaveState(EWaveState NewState);
};
