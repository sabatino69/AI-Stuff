// Fill out your copyright notice in the Description page of Project Settings.

#include "SProjectileWeapon.h"


void ASProjectileWeapon::Fire()
{

	AActor* MyOwner = GetOwner();

	if (MyOwner && ProjectileClass)
	{
		FVector EyeLoc;
		FRotator EyeRot;
		MyOwner->GetActorEyesViewPoint(EyeLoc, EyeRot);
	
		FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);
		

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLoc, EyeRot, SpawnParams);
	}
}

