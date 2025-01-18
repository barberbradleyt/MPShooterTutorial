// Fill out your copyright notice in the Description page of Project Settings.


#include "KillVolume.h"
#include "Components/BoxComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Weapon/Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/WeaponTypes.h"


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
	
	if (OtherActor->IsA(ABlasterCharacter::StaticClass()))
	{
		ABlasterCharacter* Character = Cast<ABlasterCharacter>(OtherActor);
		// Reset Weapons and Flags to their starting position
		UCombatComponent* Combat = Character->GetCombat();
		if (Combat)
		{
			if (Combat->EquippedWeapon)
			{
				FString WeaponType = UEnum::GetDisplayValueAsText(Combat->EquippedWeapon->GetWeaponType()).ToString();
				UE_LOG(LogTemp, Warning, TEXT("AKillVolume::KillOnOverlap -> Equipped Weapon type= %s"), *WeaponType);
				Combat->EquippedWeapon->ResetWeapon();
			}
			if (Combat->SecondaryWeapon)
			{
				FString WeaponType = UEnum::GetDisplayValueAsText(Combat->SecondaryWeapon->GetWeaponType()).ToString();
				UE_LOG(LogTemp, Warning, TEXT("AKillVolume::KillOnOverlap -> Secondary Weapon type= %s"), *WeaponType);
				Combat->SecondaryWeapon->ResetWeapon();
			}
			if (Combat->TheFlag)
			{
				FString WeaponType = UEnum::GetDisplayValueAsText(Combat->TheFlag->GetWeaponType()).ToString();
				UE_LOG(LogTemp, Warning, TEXT("AKillVolume::KillOnOverlap -> Flag Weapon type= %s"), *WeaponType);
				Combat->TheFlag->ResetWeapon();
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AKillVolume::KillOnOverlap -> Combat is null."));
		}
		// Apply damage to character to trigger elimination
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
	else if (OtherActor->IsA(AWeapon::StaticClass()))
	{
		AWeapon* Weapon = Cast<AWeapon>(OtherActor);

		FString WeaponType = UEnum::GetDisplayValueAsText(Weapon->GetWeaponType()).ToString();
		UE_LOG(LogTemp, Warning, TEXT("AKillVolume::KillOnOverlap -> Overlapped Weapon type= %s"), *WeaponType);

		Weapon->ResetWeapon();
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("AKillVolume::KillOnOverlap -> OtherActor is not a BlasterCharacter or AWeapon."));
	}
}

