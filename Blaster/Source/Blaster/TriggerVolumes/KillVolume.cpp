// Fill out your copyright notice in the Description page of Project Settings.


#include "KillVolume.h"
#include "Components/BoxComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"


AKillVolume::AKillVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	KillBox = CreateDefaultSubobject<UBoxComponent>(TEXT("KillBox"));
	SetRootComponent(KillBox);
}

void AKillVolume::BeginPlay()
{
	Super::BeginPlay();

	KillBox->OnComponentBeginOverlap.AddDynamic(this, &AKillVolume::KillOnOverlap);
}

void AKillVolume::KillOnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* Character = Cast<ABlasterCharacter>(OtherActor);

	if (Character)
	{
		ABlasterPlayerController* BlasterController = Cast<ABlasterPlayerController>(Character->Controller);
		if (BlasterController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, MAX_int32, BlasterController, this, UDamageType::StaticClass());
		}
		else 
		{
			UE_LOG(LogTemp, Warning, TEXT("AKillVolume::KillOnOverlap -> BlasterController is null."));
		}
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("AKillVolume::KillOnOverlap -> Character is null."));
	}
}

