// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);
}

/**
* Health Buff
*/

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || Character == nullptr || Character->isEliminated()) return;

	const float HealThisFrame = HealingRate * DeltaTime;
	
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));
	Character->UpdateHUDHealth();
	AmountToHeal -= HealThisFrame;

	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

/**
* Shield Buff
*/

void UBuffComponent::RechargeShield(float ShieldAmount, float ShieldRechargeTime)
{
	bRechargingShield = true;
	ShieldRechargeRate = ShieldAmount / ShieldRechargeTime;
	AmountToRecharge += ShieldAmount;
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (!bRechargingShield || Character == nullptr || Character->isEliminated()) return;

	const float ShieldRechargeThisFrame = ShieldRechargeRate * DeltaTime;

	Character->SetShield(FMath::Clamp(Character->GetShield() + ShieldRechargeThisFrame, 0.f, Character->GetMaxShield()));
	Character->UpdateHUDShield();
	AmountToRecharge -= ShieldRechargeThisFrame;

	if (AmountToRecharge <= 0.f || Character->GetShield() >= Character->GetMaxShield())
	{
		bRechargingShield = false;
		AmountToRecharge = 0.f;
	}
}

/**
* Speed Buff
*/

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	if(Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,
		this,
		&UBuffComponent::ResetSpeeds,
		BuffTime
	);

	if(Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}
	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

void UBuffComponent::ResetSpeeds() 
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;
	
	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}


void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

/**
* Jump Buff
*/

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
	if (Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		JumpBuffTimer,
		this,
		&UBuffComponent::ResetJump,
		BuffTime
	);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
	}
	MulticastJumpBuff(BuffJumpVelocity);
}

void UBuffComponent::ResetJump()
{
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
	}
	MulticastJumpBuff(InitialJumpVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float BuffJumpVelocity)
{
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
	}
}

void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
	InitialJumpVelocity = Velocity;
}

/**
* Invisibility Buff
*/

void UBuffComponent::BuffInvisibility(float BuffTime)
{
	if (Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		InvisibilityBuffTimer,
		this,
		&UBuffComponent::ResetInvisibilityMaterial,
		BuffTime
	);

	if (InvisibilityMaterial && Character->GetMesh())
	{
		Character->GetMesh()->SetMaterial(0, InvisibilityMaterial);
	}

	MulticastSetCharacterInvisible();
}

void UBuffComponent::ResetInvisibilityMaterial()
{
	if (InitialMaterial && Character && Character->GetMesh())
	{
		Character->GetMesh()->SetMaterial(0, InitialMaterial);
	}

	MulticastResetCharacterMaterial();
}

void UBuffComponent::MulticastSetCharacterInvisible_Implementation()
{
	if (InvisibilityMaterial && Character && Character->GetMesh())
	{
		Character->GetMesh()->SetMaterial(0, InvisibilityMaterial);
	}
}

void UBuffComponent::MulticastResetCharacterMaterial_Implementation()
{
	if (InitialMaterial && Character && Character->GetMesh())
	{
		Character->GetMesh()->SetMaterial(0, InitialMaterial);
	}
}

void UBuffComponent::SetInitialMaterial(UMaterialInstance* Material)
{
	InitialMaterial = Material;
}
