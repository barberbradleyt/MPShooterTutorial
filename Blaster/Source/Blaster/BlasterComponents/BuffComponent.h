// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	friend class ABlasterCharacter;
	void Heal(float HealAmount, float HealingTime);
	void RechargeShield(float ShieldAmount, float ShieldRechargeTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffJump(float BuffJumpVelocity, float BuffTime);
	void BuffInvisibility(float BuffTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float Velocity);
	void SetInitialMaterial(UMaterialInstance* Material);
protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);
private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	/**
	* Heal Buff
	*/

	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	/**
	* Shield Buff
	*/

	bool bRechargingShield = false;
	float ShieldRechargeRate = 0.f;
	float AmountToRecharge = 0.f;

	/**
	* Speed Buff
	*/

	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds();
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/**
	* Jump Buff
	*/

	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float BuffJumpVelocity);

	/**
	* Invisibility Buff
	*/

	FTimerHandle InvisibilityBuffTimer;
	void ResetInvisibilityMaterial();
	
	UPROPERTY(EditAnywhere, Category = InvisibilityBuff)
	UMaterialInstance* InvisibilityMaterial;

	UPROPERTY()
	UMaterialInstance* InitialMaterial;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetCharacterInvisible();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetCharacterMaterial();

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
