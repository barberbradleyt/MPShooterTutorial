// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float DeltaTime = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / DeltaTime, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];
		
		FBoxInformation InterpBoxInfo;
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(
	const FFramePackage& Package,
	ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation)
{
	if (Package.Character == nullptr) return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CurrentFrame.Character = Package.Character;
	CacheBoxPositions(Package.Character, CurrentFrame);
	MoveBoxes(Package.Character, Package);
	EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::NoCollision);

	// Enable collision for the head first
	UBoxComponent* HeadBox = Package.Character->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;

	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	UWorld* World = GetWorld();
	if (World == nullptr) return FServerSideRewindResult();

	World->LineTraceSingleByChannel(
		ConfirmHitResult,
		TraceStart,
		TraceEnd,
		ECollisionChannel::ECC_Visibility
	);
	//TODO refactor
	if (ConfirmHitResult.bBlockingHit) // Hit head, return early
	{
		ResetHitBoxes(Package.Character, CurrentFrame);
		EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}
	else 
	{
		for (auto& HitBoxPair : Package.Character->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
				
			}
		}
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Visibility
		);
		if (ConfirmHitResult.bBlockingHit) 
		{
			ResetHitBoxes(Package.Character, CurrentFrame);
			EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ true, false };
		}
		else 
		{
			ResetHitBoxes(Package.Character, CurrentFrame);
			EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ false, false };
		}
	}
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(
	const TArray<FFramePackage>& Packages,
	const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations)
{
	for (FFramePackage Package : Packages)
	{
		if (Package.Character == nullptr) return FShotgunServerSideRewindResult();
	}

	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (FFramePackage Package : Packages)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Package.Character;
		CacheBoxPositions(Package.Character, CurrentFrame);
		MoveBoxes(Package.Character, Package);
		EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);
	}

	for (FFramePackage Package : Packages)
	{
		// Enable collision for the head first
		UBoxComponent* HeadBox = Package.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	}

	UWorld* World = GetWorld();
	if (World == nullptr) return FShotgunServerSideRewindResult();
	// Check for head shots
	for (FVector_NetQuantize HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Visibility
		);
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
		if (BlasterCharacter)
		{
			if (ShotgunResult.HeadShots.Contains(BlasterCharacter))
			{
				ShotgunResult.HeadShots[BlasterCharacter]++;
			}
			else {
				ShotgunResult.HeadShots.Emplace(BlasterCharacter, 1);
			}
		}
	}

	// Enable collision for all boxes except head boxes
	for (FFramePackage Package : Packages)
	{
		for (auto& HitBoxPair : Package.Character->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

			}
		}
		// Disable collision for the head first
		UBoxComponent* HeadBox = Package.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Check for body shots
	for (FVector_NetQuantize HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Visibility
		);
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
		if (BlasterCharacter)
		{
			if (ShotgunResult.BodyShots.Contains(BlasterCharacter))
			{
				ShotgunResult.BodyShots[BlasterCharacter]++;
			}
			else {
				ShotgunResult.BodyShots.Emplace(BlasterCharacter, 1);
			}
		}
	}

	for (FFramePackage CurrentFrame : CurrentFrames)
	{
		ResetHitBoxes(CurrentFrame.Character, CurrentFrame);
		EnableCharacterMeshCollision(CurrentFrame.Character, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) 
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr) 
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Colour)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(), 
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Colour,
			false,
			4.f
		);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(class ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation,
	float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	const TArray<ABlasterCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations,
	float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
	if (bReturn) return FFramePackage();

	// Frame history of HitCharacter
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;

	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (OldestHistoryTime > HitTime) return FFramePackage(); // Too far back - too laggy to do server side rewind

	FFramePackage FrameToCheck;
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
	}
	else if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
	}
	else // Iterate over frames to find or interpolate frame to check
	{
		TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
		TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
		while (Older->GetValue().Time > HitTime)
		{
			// March back until:  Older.Time < HitTime < Younger.Time
			if (Older->GetNextNode() == nullptr) break;
			Older = Older->GetNextNode();
			if (Older->GetValue().Time > HitTime)
			{
				Younger = Older;
			}
		}

		if (Older->GetValue().Time == HitTime)
		{
			FrameToCheck = Older->GetValue();
		}
		else
		{
			FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
		}
	}

	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(
	ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation,
	float HitTime,
	class AWeapon* DamageCauser
)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && HitCharacter && DamageCauser && Confirm.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter, 
			DamageCauser->GetDamage(), 
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(
	const TArray<ABlasterCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations,
	float HitTime,
	class AWeapon* DamageCauser // May want to use Character.GetEquippedWeapon() to use server-confirmed weapon
)
{
	if (Character == nullptr || DamageCauser == nullptr) return;

	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr) return;

		float TotalDamage = 0.f;
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			TotalDamage += Confirm.HeadShots[HitCharacter] * DamageCauser->GetDamage(); //TODO add head shot multiplier
		}
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			TotalDamage += Confirm.BodyShots[HitCharacter] * DamageCauser->GetDamage();
		}
		
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority()) return;

	FFramePackage ThisFrame;
	SaveFramePackage(ThisFrame);
	FrameHistory.AddHead(ThisFrame);

	float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
	while (HistoryLength > MaxRecordTime)
	{
		FrameHistory.RemoveNode(FrameHistory.GetTail());
		HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
	}

	//ShowFramePackage(ThisFrame, FColor::Magenta);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}