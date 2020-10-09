// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(TEXT("Coop.DebugWeapons"), DebugWeaponDrawing, TEXT("Draw Debug Lines for Weapons"), ECVF_Cheat);


// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TraceTargetName = "BeamEnd";
	BaseDamage = 20.f;

	RateOfFire = 600.f;
	BulletSpread = 2.f;
	SetReplicates(true);

	NetUpdateFrequency = 60.f;
	MinNetUpdateFrequency = 33.f;

}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

void ASWeapon::Fire()
{
	if (Role < ROLE_Authority)
	{
		ServerFire();
		
	}
	
	// trace the world from pawn eyes to crosshair location
	
	AActor* MyOwner = GetOwner();

	if (MyOwner)
	{
		FVector EyeLoc;
		FRotator EyeRot;
		MyOwner->GetActorEyesViewPoint(EyeLoc, EyeRot);

		FVector ShotDir = EyeRot.Vector();
		
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		
		
		ShotDir = FMath::VRandCone(ShotDir, HalfRad, HalfRad);
		
		FVector TraceEnd = EyeLoc + (ShotDir * 10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;
		// particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLoc, TraceEnd, ECC_GameTraceChannel1, QueryParams))
		{
			//hit! process damage
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
			
			float ActualDamage = BaseDamage;
			if (SurfaceType == SurfaceType2)
			{
				ActualDamage *= 4.f;
			}

 			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDir, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			PlayImpactFX(SurfaceType,Hit.ImpactPoint);

		
			TracerEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), EyeLoc, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
		}

		PlayFireFX(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.Surfacetype = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::OnRep_HitScanTrace()
{
	PlayFireFX(HitScanTrace.TraceTo);

	PlayImpactFX(HitScanTrace.Surfacetype, HitScanTrace.TraceTo);

}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds,0.0f);

	GetWorldTimerManager().SetTimer(TH_TimeBetweenShots,this,&ASWeapon::Fire,TimeBetweenShots,true,FirstDelay);
	
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TH_TimeBetweenShots);
}



void ASWeapon::PlayFireFX(FVector TracerEndPoint)
{
	if (MuzzleFX)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFX, MeshComp, MuzzleSocketName);
	}

	if (TracerFX)
	{
		FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerFX, MuzzleLoc);

		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TraceTargetName, TracerEndPoint);

		}
	}
	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}

void ASWeapon::PlayImpactFX(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;

	switch (SurfaceType)
	{
	case SurfaceType1: //flesh default
		SelectedEffect = FleshImpactFX;
		break;
	case SurfaceType2: //flesh vulnerable
		SelectedEffect = FleshImpactFX;
		break;
	default:
		SelectedEffect = DefaultImpactFX;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLoc;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}

}


void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace,COND_SkipOwner);
}
