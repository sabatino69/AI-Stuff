// Fill out your copyright notice in the Description page of Project Settings.

#include "STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/NavigationSystem/Public/NavigationSystem.h"
#include "Runtime/NavigationSystem/Public/NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "SHealthComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "SCharacter.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"

static int32 DebugTrackerBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugTrackerBotDrawing(TEXT("Coop.DebugTrackerBot"), DebugTrackerBotDrawing, TEXT("Draw Debug Lines for TrackerBot"), ECVF_Cheat);

// Sets default values
ASTrackerBot::ASTrackerBot()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Meshcomp"));
	MeshComp->SetCanEverAffectNavigation(false);
	
	MeshComp->SetSimulatePhysics(true);
	
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("Healthcomp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("Spherecomp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);

	bUseVelocityChange = false;
	MovementForce = 1000.f;
	RequireDistanceToTarget = 100.f;

	ExplosionDamage = 60.f;
	ExplosionRadius = 350.f;

	SelfDamageInterval = 0.25f;
	
	SetReplicates(true);
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	if (Role == ROLE_Authority)
	{
		NextPathPoint = GetNextPathPoint();

		FTimerHandle TimerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearByBots, 1.f, true);
	}
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType*
	DamageType, class AController* InitigatedBy, AActor* DamageCauser)
{
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);

	}

	UE_LOG(LogTemp, Log, TEXT("Health %s of %s"), *FString::SanitizeFloat(Health), *GetName());

	if (Health <= 0.f)
	{

		SelfDestruct();
	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	AActor* BestTarget = nullptr;
	float NearestTargetDistanace = FLT_MAX;


	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		if (TestPawn == nullptr || USHealthComponent::IsFriendly(TestPawn, this))
		{
			continue;
		}

		USHealthComponent* TestPawnHealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.f)
		{
			float Distance = (TestPawn->GetActorLocation() - GetActorLocation()).Size();

			if (Distance < NearestTargetDistanace)
			{
				BestTarget = TestPawn;
				NearestTargetDistanace = Distance;
			}

		}

	}

	if (BestTarget)
	{
		

		UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

		GetWorldTimerManager().ClearTimer(TH_RefreshPath);
		GetWorldTimerManager().SetTimer(TH_RefreshPath, this, &ASTrackerBot::RefreshPath, 5.0, false);

		if (NavPath && NavPath->PathPoints.Num() > 1)
		{
			return NavPath->PathPoints[1];
		}
	}
	

	return GetActorLocation();
}




void ASTrackerBot::SelfDestruct()
{

	if (bExploded)
	{

		return;
	}

	bExploded = true;
	OnRep_Exploded();
	

	

	MeshComp->SetVisibility(false, true);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	if (Role == ROLE_Authority)
	{
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		UGameplayStatics::ApplyRadialDamage(this, ActualDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);


		if (DebugTrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Purple, false, 2.f, 0, 1.f);
		}
		
		SetLifeSpan(.25f);
	}

	
}

void ASTrackerBot::OnRep_Exploded()
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionFX, GetActorLocation());

	UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
}

void ASTrackerBot::DamageSelf()
{

	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::OnCheckNearByBots()
{

	const float Radius = 600.f;

	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	FCollisionObjectQueryParams QueryParams;

	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);

	if (DebugTrackerBotDrawing)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.f);
	}

	int32 NrOfBots = 0;
	for(FOverlapResult Result : Overlaps)
	{
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());
		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;

	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);

	if (MatInst == nullptr)
	{

		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));

	}
	if(MatInst)
	{

		float Alpha = PowerLevel / (float)MaxPowerLevel;
		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}

	if (DebugTrackerBotDrawing)
	{
		DrawDebugString(GetWorld(), FVector(0, 0, 0), FString::FromInt(PowerLevel), this, FColor::White, 1.f, true);
	}
}

void ASTrackerBot::RefreshPath()
{

	NextPathPoint = GetNextPathPoint();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role == ROLE_Authority && !bExploded)
	{

		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget <= RequireDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();

			if (DebugTrackerBotDrawing)
			{
				DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached!");
			}

		}
		else
		{
			FVector ForceDir = NextPathPoint - GetActorLocation();
			
			ForceDir.Normalize();
			
			ForceDir *= MovementForce;
			
			
			
			MeshComp->AddForce(ForceDir, NAME_None, bUseVelocityChange);
			
			if (DebugTrackerBotDrawing)
			{
				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDir, 32, FColor::Red, false, 0, 0, 1.f);
			}

		}

		if (DebugTrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), NextPathPoint, 20.f, 12.f, FColor::Red, false, 0, 1.f);
		}

	}
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor * OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bStartedSelfDestruct && !bExploded)
	{
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);

		if (PlayerPawn && !USHealthComponent::IsFriendly(OtherActor, this))
		{
			if (Role == ROLE_Authority)
			{
				GetWorldTimerManager().SetTimer(TimerHandle_selfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.f);
			}
			bStartedSelfDestruct = true;

			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}
}

void ASTrackerBot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASTrackerBot, bExploded);

}



