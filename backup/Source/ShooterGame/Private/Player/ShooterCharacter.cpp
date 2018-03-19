// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#define COLLISION_HITBOX ECC_GameTraceChannel4

#include "ShooterGame.h"
#include "Weapons/ShooterWeapon.h"
#include "Weapons/ShooterDamageType.h"
#include "UI/ShooterHUD.h"
#include "Online/ShooterPlayerState.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundNodeLocalPlayer.h"
#include "AudioThread.h"

static int32 NetVisualizeRelevancyTestPoints = 0;
FAutoConsoleVariableRef CVarNetVisualizeRelevancyTestPoints(
	TEXT("p.NetVisualizeRelevancyTestPoints"),
	NetVisualizeRelevancyTestPoints,
	TEXT("")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);


static int32 NetEnablePauseRelevancy = 1;
FAutoConsoleVariableRef CVarNetEnablePauseRelevancy(
	TEXT("p.NetEnablePauseRelevancy"),
	NetEnablePauseRelevancy,
	TEXT("")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

AShooterCharacter::AShooterCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("PawnMesh1P"));
	Mesh1P->SetupAttachment(GetCapsuleComponent());
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	Mesh1P->SetCollisionObjectType(ECC_Pawn);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);

	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	GenerateHitboxes();

	TargetingSpeedModifier = 0.5f;
	bIsTargeting = false;
	RunningSpeedModifier = 1.5f;
	bWantsToRun = false;
	bWantsToFire = false;
	LowHealthPercentage = 0.5f;

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;


	ShooterCharacterMovement = Cast<UShooterCharacterMovement>(GetCharacterMovement());

}

// Begin lag compensation code
void AShooterCharacter::GenerateHitboxes() {
	USkeletalMeshComponent* PlayerMesh = GetMesh();
	
	
	// HB_Head
	HB_Head = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_Head"));
	HB_Head->SetupAttachment(PlayerMesh, FName(TEXT("b_head")));
	HB_Head->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
	HB_Head->SetRelativeRotation(FRotator(0.0f, 22.5f, 0.0f));
	HB_Head->SetWorldScale3D(FVector(0.28125f, 0.3125f, 0.375f));

	// COLLISION_HITBOX is a custom collision channel
	HB_Head->SetCollisionObjectType(COLLISION_HITBOX);
	HB_Head->SetCollisionProfileName("Hitbox");
	HB_Head->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_Head->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	
	// HB_UpperTorso
	HB_UpperTorso = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_UpperTorso"));
	HB_UpperTorso->SetupAttachment(PlayerMesh, FName(TEXT("b_Spine1")));
	HB_UpperTorso->SetRelativeLocation(FVector(0.0f, -13.0f, 0.0f));
	HB_UpperTorso->SetRelativeRotation(FRotator(0.f, 0.0f, 0.f));
	HB_UpperTorso->SetWorldScale3D(FVector(0.5625f, 0.5f, 0.6875f));

	// COLLISION_HITBOX is a custom collision channel
	HB_UpperTorso->SetCollisionObjectType(COLLISION_HITBOX);
	HB_UpperTorso->SetCollisionProfileName("Hitbox");
	HB_UpperTorso->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_UpperTorso->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	
	// HB_LowerTorso
	HB_LowerTorso = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_LowerTorso"));
	HB_LowerTorso->SetupAttachment(PlayerMesh, FName(TEXT("b_Hips")));
	HB_LowerTorso->SetRelativeLocation(FVector(0.0f, -13.0f, 0.0f));
	HB_LowerTorso->SetRelativeRotation(FRotator(0.f, 0.0f, 0.f));
	HB_LowerTorso->SetWorldScale3D(FVector(0.46875f, 0.65625f, 0.65625f));

	// COLLISION_HITBOX is a custom collision channel
	HB_LowerTorso->SetCollisionObjectType(COLLISION_HITBOX);
	HB_LowerTorso->SetCollisionProfileName("Hitbox");
	HB_LowerTorso->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_LowerTorso->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	
	// HB_LowerLeftLeg
	HB_LowerLeftLeg = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_LowerLeftLeg"));
	HB_LowerLeftLeg->SetupAttachment(PlayerMesh, FName(TEXT("b_LeftLeg")));
	HB_LowerLeftLeg->SetRelativeLocation(FVector(16.0f, 0.0f, 0.0f));
	HB_LowerLeftLeg->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	HB_LowerLeftLeg->SetWorldScale3D(FVector(0.78125f, 0.5f, 0.3125f));
	
	// COLLISION_HITBOX is a custom collision channel
	HB_LowerLeftLeg->SetCollisionObjectType(COLLISION_HITBOX);
	HB_LowerLeftLeg->SetCollisionProfileName("Hitbox");
	HB_LowerLeftLeg->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_LowerLeftLeg->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	// HB_UpperLeftLeg
	HB_UpperLeftLeg = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_UpperLeftLeg"));
	HB_UpperLeftLeg->SetupAttachment(PlayerMesh, FName(TEXT("b_LeftUpLeg")));
	HB_UpperLeftLeg->SetRelativeLocation(FVector(26.999998f, 0.0f, 0.0f));
	HB_UpperLeftLeg->SetRelativeRotation(FRotator(0.f, 0.0f, 0.f));
	HB_UpperLeftLeg->SetWorldScale3D(FVector(0.78125f, 0.5625f, 0.3125f));

	// COLLISION_HITBOX is a custom collision channel
	HB_UpperLeftLeg->SetCollisionObjectType(COLLISION_HITBOX);
	HB_UpperLeftLeg->SetCollisionProfileName("Hitbox");
	HB_UpperLeftLeg->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_UpperLeftLeg->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	
	
	// HB_LeftFoot
	HB_LeftFoot = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_LeftFoot"));
	HB_LeftFoot->SetupAttachment(PlayerMesh, FName(TEXT("b_LeftFoot")));
	HB_LeftFoot->SetRelativeLocation(FVector(8.794228f, -2.767963f, 0.0f));
	HB_LeftFoot->SetRelativeRotation(FRotator(0.0f, -30.000092f, 0.0f));
	HB_LeftFoot->SetWorldScale3D(FVector(0.40625f, 0.28125f, 0.34375f));

	// COLLISION_HITBOX is a custom collision channel
	HB_LeftFoot->SetCollisionObjectType(COLLISION_HITBOX);
	HB_LeftFoot->SetCollisionProfileName("Hitbox");
	HB_LeftFoot->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_LeftFoot->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	
	// HB_RightFoot
	HB_RightFoot = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_RightFoot"));
	HB_RightFoot->SetupAttachment(PlayerMesh, FName(TEXT("b_RightFoot")));
	HB_RightFoot->SetRelativeLocation(FVector(-8.794228f, 2.767963f, 0.0f));
	HB_RightFoot->SetRelativeRotation(FRotator(0.f, -30.000092f, 0.0f));
	HB_RightFoot->SetWorldScale3D(FVector(0.40625f, 0.28125f, 0.34375f));

	// COLLISION_HITBOX is a custom collision channel
	HB_RightFoot->SetCollisionObjectType(COLLISION_HITBOX);
	HB_RightFoot->SetCollisionProfileName("Hitbox");
	HB_RightFoot->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_RightFoot->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	
	// HB_LowerRightLeg
	HB_LowerRightLeg = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_LowerRightLeg"));
	HB_LowerRightLeg->SetupAttachment(PlayerMesh, FName(TEXT("b_RightLeg")));
	HB_LowerRightLeg->SetRelativeLocation(FVector(-16.0f, 0.0f, 0.0f));
	HB_LowerRightLeg->SetRelativeRotation(FRotator(0.f, 0.0f, 0.f));
	HB_LowerRightLeg->SetWorldScale3D(FVector(0.78125f, 0.5f, 0.3125f));

	// COLLISION_HITBOX is a custom collision channel
	HB_LowerRightLeg->SetCollisionObjectType(COLLISION_HITBOX);
	HB_LowerRightLeg->SetCollisionProfileName("Hitbox");
	HB_LowerRightLeg->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_LowerRightLeg->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	
	// HB_UpperRightLeg
	HB_UpperRightLeg = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_UpperRightLeg"));
	HB_UpperRightLeg->SetupAttachment(PlayerMesh, FName(TEXT("b_RightUpLeg")));
	HB_UpperRightLeg->SetRelativeLocation(FVector(-26.999998f, 0.0f, 0.0f));
	HB_UpperRightLeg->SetRelativeRotation(FRotator(0.f, 0.0f, 0.f));
	HB_UpperRightLeg->SetWorldScale3D(FVector(0.78125f, 0.5625f, 0.3125f));

	// COLLISION_HITBOX is a custom collision channel
	HB_UpperRightLeg->SetCollisionObjectType(COLLISION_HITBOX);
	HB_UpperRightLeg->SetCollisionProfileName("Hitbox");
	HB_UpperRightLeg->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_UpperRightLeg->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	// HB_LeftHand
	HB_LeftHand = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_LeftHand"));
	HB_LeftHand->SetupAttachment(PlayerMesh, FName(TEXT("b_LeftHand")));
	HB_LeftHand->SetRelativeLocation(FVector(10.548982f, 3.11426f, 0.0f));
	HB_LeftHand->SetRelativeRotation(FRotator(0.f, 8.437527f, 0.f));
	HB_LeftHand->SetWorldScale3D(FVector(0.3125f, 0.28125f, 0.1875f));

	// COLLISION_HITBOX is a custom collision channel
	HB_LeftHand->SetCollisionObjectType(COLLISION_HITBOX);
	HB_LeftHand->SetCollisionProfileName("Hitbox");
	HB_LeftHand->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_LeftHand->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	// HB_LowerLeftArm
	HB_LowerLeftArm = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_LowerLeftArm"));
	HB_LowerLeftArm->SetupAttachment(PlayerMesh, FName(TEXT("b_LeftForeArm")));
	HB_LowerLeftArm->SetRelativeLocation(FVector(15.948524f, 0.0f, 2.90307f));
	HB_LowerLeftArm->SetRelativeRotation(FRotator(0.f, 0.0f, -2.812469f));
	HB_LowerLeftArm->SetWorldScale3D(FVector(0.5625f, 0.375f, 0.28125f));

	// COLLISION_HITBOX is a custom collision channel
	HB_LowerLeftArm->SetCollisionObjectType(COLLISION_HITBOX);
	HB_LowerLeftArm->SetCollisionProfileName("Hitbox");
	HB_LowerLeftArm->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_LowerLeftArm->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	
	// HB_UpperLeftArm
	HB_UpperLeftArm = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_UpperLeftArm"));
	HB_UpperLeftArm->SetupAttachment(PlayerMesh, FName(TEXT("b_LeftArm")));
	HB_UpperLeftArm->SetRelativeLocation(FVector(8.0f, 0.0f, 4.0f));
	HB_UpperLeftArm->SetRelativeRotation(FRotator(0.f, 0.0f, 0.f));
	HB_UpperLeftArm->SetWorldScale3D(FVector(0.71875f, 0.28125f, 0.34375f));

	// COLLISION_HITBOX is a custom collision channel
	HB_UpperLeftArm->SetCollisionObjectType(COLLISION_HITBOX);
	HB_UpperLeftArm->SetCollisionProfileName("Hitbox");
	HB_UpperLeftArm->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_UpperLeftArm->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	
	// HB_RightHand
	HB_RightHand = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_RightHand"));
	HB_RightHand->SetupAttachment(PlayerMesh, FName(TEXT("b_RightHand")));
	HB_RightHand->SetRelativeLocation(FVector(-10.548982f, -3.11426f, 0.0f));
	HB_RightHand->SetRelativeRotation(FRotator(0.f, 8.437527f, 0.f));
	HB_RightHand->SetWorldScale3D(FVector(0.3125f, 0.28125f, 0.1875f));

	// COLLISION_HITBOX is a custom collision channel
	HB_RightHand->SetCollisionObjectType(COLLISION_HITBOX);
	HB_RightHand->SetCollisionProfileName("Hitbox");
	HB_RightHand->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_RightHand->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	// HB_LowerRightArm
	HB_LowerRightArm = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_LowerRightArm"));
	HB_LowerRightArm->SetupAttachment(PlayerMesh, FName(TEXT("b_RightForeArm")));
	HB_LowerRightArm->SetRelativeLocation(FVector(-15.948524f, 0.0f, -2.90307f));
	HB_LowerRightArm->SetRelativeRotation(FRotator(-2.812469f, 0.0f, 0.0f));
	HB_LowerRightArm->SetWorldScale3D(FVector(0.5625f, 0.375f, 0.28125f));

	// COLLISION_HITBOX is a custom collision channel
	HB_LowerRightArm->SetCollisionObjectType(COLLISION_HITBOX);
	HB_LowerRightArm->SetCollisionProfileName("Hitbox");
	HB_LowerRightArm->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_LowerRightArm->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	
	// HB_UpperRightArm
	HB_UpperRightArm = CreateDefaultSubobject<UBoxComponent>(TEXT("HB_UpperRightArm"));
	HB_UpperRightArm->SetupAttachment(PlayerMesh, FName(TEXT("b_RightArm")));
	HB_UpperRightArm->SetRelativeLocation(FVector(-8.0f, 0.0f, -4.0f));
	HB_UpperRightArm->SetRelativeRotation(FRotator(0.f, 0.0f, 0.f));
	HB_UpperRightArm->SetWorldScale3D(FVector(0.71875f, 0.28125f, 0.34375f));

	// COLLISION_HITBOX is a custom collision channel
	HB_UpperRightArm->SetCollisionObjectType(COLLISION_HITBOX);
	HB_UpperRightArm->SetCollisionProfileName("Hitbox");
	HB_UpperRightArm->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HB_UpperRightArm->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
}

void AShooterCharacter::PositionUpdated() {

	const FVector LocationToSave = GetActorLocation();
	const FRotator RotationToSave = GetViewRotation();
	const float WorldTime = GetWorld()->GetTimeSeconds();
	const TArray<FSavedHitbox> HitboxesToSave = BuildSavedHitboxArr();

	const FSavedPosition PositionToSave = FSavedPosition(
		LocationToSave,
		RotationToSave,
		WorldTime,
		HitboxesToSave
	);

	SavedPositions.Add(PositionToSave);

	// Clean up SavedPositions that have exceeded out Age limit.
	// However, we should keep a handle to at least one FSavedPosition that exceeds the max age
	// for interpolation.
	if (SavedPositions.Num() >= 2 && SavedPositions[1].Time < WorldTime - MaxSavedPositionAge) {
		SavedPositions.RemoveAt(0);
	}

}

TArray<FSavedHitbox> AShooterCharacter::BuildSavedHitboxArr() {

	TArray<FSavedHitbox> SavedHitboxArr;
	
	
	if (HB_Head) {
		FSavedHitbox SH_Head;
		SH_Head.HitboxType = EHitboxType::Head;
		SH_Head.Position = HB_Head->GetComponentLocation();
		SH_Head.Rotation = HB_Head->GetComponentRotation();
		SavedHitboxArr.Add(SH_Head);
	}

	
	if (HB_UpperTorso) {
		FSavedHitbox SH_UpperTorso;
		SH_UpperTorso.HitboxType = EHitboxType::UpperTorso;
		SH_UpperTorso.Position = HB_UpperTorso->GetComponentLocation();
		SH_UpperTorso.Rotation = HB_UpperTorso->GetComponentRotation();
		SavedHitboxArr.Add(SH_UpperTorso);
	}
	
	
	if (HB_LowerTorso) {
		FSavedHitbox SH_LowerTorso;
		SH_LowerTorso.HitboxType = EHitboxType::LowerTorso;
		SH_LowerTorso.Position = HB_LowerTorso->GetComponentLocation();
		SH_LowerTorso.Rotation = HB_LowerTorso->GetComponentRotation();
		SavedHitboxArr.Add(SH_LowerTorso);
	}

	
	if (HB_LowerLeftLeg) {
		FSavedHitbox SH_LowerLeftLeg;
		SH_LowerLeftLeg.HitboxType = EHitboxType::LowerLeftLeg;
		SH_LowerLeftLeg.Position = HB_LowerLeftLeg->GetComponentLocation();
		SH_LowerLeftLeg.Rotation = HB_LowerLeftLeg->GetComponentRotation();
		SavedHitboxArr.Add(SH_LowerLeftLeg);
	}

	
	if (HB_UpperLeftLeg) {
		FSavedHitbox SH_UpperLeftLeg;
		SH_UpperLeftLeg.HitboxType = EHitboxType::LowerLeftLeg;
		SH_UpperLeftLeg.Position = HB_UpperLeftLeg->GetComponentLocation();
		SH_UpperLeftLeg.Rotation = HB_UpperLeftLeg->GetComponentRotation();
		SavedHitboxArr.Add(SH_UpperLeftLeg);
	}

	
	if (HB_LeftFoot) {
		FSavedHitbox SH_LeftFoot;
		SH_LeftFoot.HitboxType = EHitboxType::LeftFoot;
		SH_LeftFoot.Position = HB_LeftFoot->GetComponentLocation();
		SH_LeftFoot.Rotation = HB_LeftFoot->GetComponentRotation();
		SavedHitboxArr.Add(SH_LeftFoot);
	}

	
	if (HB_RightFoot) {
		FSavedHitbox SH_RightFoot;
		SH_RightFoot.HitboxType = EHitboxType::RightFoot;
		SH_RightFoot.Position = HB_RightFoot->GetComponentLocation();
		SH_RightFoot.Rotation = HB_RightFoot->GetComponentRotation();
		SavedHitboxArr.Add(SH_RightFoot);
	}

	
	if (HB_LowerRightLeg) {
		FSavedHitbox SH_LowerRightLeg;
		SH_LowerRightLeg.HitboxType = EHitboxType::LowerRightLeg;
		SH_LowerRightLeg.Position = HB_LowerRightLeg->GetComponentLocation();
		SH_LowerRightLeg.Rotation = HB_LowerRightLeg->GetComponentRotation();
		SavedHitboxArr.Add(SH_LowerRightLeg);
	}

	
	if (HB_UpperRightLeg) {
		FSavedHitbox SH_UpperRightLeg;
		SH_UpperRightLeg.HitboxType = EHitboxType::UpperRightLeg;
		SH_UpperRightLeg.Position = HB_UpperRightLeg->GetComponentLocation();
		SH_UpperRightLeg.Rotation = HB_UpperRightLeg->GetComponentRotation();
		SavedHitboxArr.Add(SH_UpperRightLeg);
	}

	
	if (HB_LeftHand) {
		FSavedHitbox SH_LeftHand;
		SH_LeftHand.HitboxType = EHitboxType::LeftHand;
		SH_LeftHand.Position = HB_LeftHand->GetComponentLocation();
		SH_LeftHand.Rotation = HB_LeftHand->GetComponentRotation();
		SavedHitboxArr.Add(SH_LeftHand);
	}
	
	if (HB_LowerLeftArm) {
		FSavedHitbox SH_LowerLeftArm;
		SH_LowerLeftArm.HitboxType = EHitboxType::LowerLeftArm;
		SH_LowerLeftArm.Position = HB_LowerLeftArm->GetComponentLocation();
		SH_LowerLeftArm.Rotation = HB_LowerLeftArm->GetComponentRotation();
		SavedHitboxArr.Add(SH_LowerLeftArm);
	}
	
	if (HB_UpperLeftArm) {
		FSavedHitbox SH_UpperLeftArm;
		SH_UpperLeftArm.HitboxType = EHitboxType::UpperLeftArm;
		SH_UpperLeftArm.Position = HB_UpperLeftArm->GetComponentLocation();
		SH_UpperLeftArm.Rotation = HB_UpperLeftArm->GetComponentRotation();
		SavedHitboxArr.Add(SH_UpperLeftArm);
	}
	
	
	if (HB_RightHand) {
		FSavedHitbox SH_RightHand;
		SH_RightHand.HitboxType = EHitboxType::RightHand;
		SH_RightHand.Position = HB_RightHand->GetComponentLocation();
		SH_RightHand.Rotation = HB_RightHand->GetComponentRotation();
		SavedHitboxArr.Add(SH_RightHand);
	}

	if (HB_LowerRightArm) {
		FSavedHitbox SH_LowerRightArm;
		SH_LowerRightArm.HitboxType = EHitboxType::LowerRightArm;
		SH_LowerRightArm.Position = HB_LowerRightArm->GetComponentLocation();
		SH_LowerRightArm.Rotation = HB_LowerRightArm->GetComponentRotation();
		SavedHitboxArr.Add(SH_LowerRightArm);
	}
	
	if (HB_UpperRightArm) {
		FSavedHitbox SH_UpperRightArm;
		SH_UpperRightArm.HitboxType = EHitboxType::UpperRightArm;
		SH_UpperRightArm.Position = HB_UpperRightArm->GetComponentLocation();
		SH_UpperRightArm.Rotation = HB_UpperRightArm->GetComponentRotation();
		SavedHitboxArr.Add(SH_UpperRightArm);
	}
	return SavedHitboxArr;

}

UBoxComponent* AShooterCharacter::GetHitbox(EHitboxType HitboxType) {

	UBoxComponent* Hitbox = nullptr;

	switch (HitboxType) {

	
	case EHitboxType::Head:
		Hitbox = HB_Head;
		break;

	
	case EHitboxType::UpperTorso:
		Hitbox = HB_UpperTorso;
		break;

	
	case EHitboxType::LowerTorso:
		Hitbox = HB_LowerTorso;
		break;
	
	
	case EHitboxType::LowerLeftLeg:
		Hitbox = HB_LowerLeftLeg;
		break;

	
	case EHitboxType::UpperLeftLeg:
		Hitbox = HB_UpperLeftLeg;
		break;

	
	case EHitboxType::LeftFoot:
		Hitbox = HB_LeftFoot;
		break;

	
	case EHitboxType::RightFoot:
		Hitbox = HB_RightFoot;
		break;

	
	case EHitboxType::LowerRightLeg:
		Hitbox = HB_LowerRightLeg;
		break;

	
	case EHitboxType::UpperRightLeg:
		Hitbox = HB_UpperRightLeg;
		break;

	
	case EHitboxType::LeftHand:
		Hitbox = HB_LeftHand;
		break;

	case EHitboxType::LowerLeftArm:
		Hitbox = HB_LowerLeftArm;
		break;
	
	case EHitboxType::UpperLeftArm:
		Hitbox = HB_UpperLeftArm;
		break;
	
	
	case EHitboxType::RightHand:
		Hitbox = HB_RightHand;
		break;

	case EHitboxType::LowerRightArm:
		Hitbox = HB_LowerRightArm;
		break;
		
	case EHitboxType::UpperRightArm:
		Hitbox = HB_UpperRightArm;
		break;
		
	default:
		break;
	}

	return Hitbox;

}


FVector AShooterCharacter::GetHitboxExtent(EHitboxType HitboxType) {

	UBoxComponent* Hitbox = GetHitbox(HitboxType);

	if (Hitbox) {
		return Hitbox->GetScaledBoxExtent();
	}

	return FVector(0.f);

}

void AShooterCharacter::DrawSavedPositions(const TArray<FSavedPosition> SavedPositions) {

	const FColor BoxColor = FColor::Green;
	const float BoxLifetime = 0.1f;
	const uint8 DepthPriority = 0;
	const float BoxThickness = 0.75f;
	const bool PersistentLines = true;

	for (FSavedPosition SavedPosition : SavedPositions) {
		for (FSavedHitbox SavedHitbox : SavedPosition.Hitboxes) {
			DrawDebugBox(
				GetWorld(),
				SavedHitbox.Position,
				GetHitboxExtent(SavedHitbox.HitboxType),
				SavedHitbox.Rotation.Quaternion(),
				BoxColor,
				PersistentLines,
				BoxLifetime,
				DepthPriority,
				PersistentLines
			);
		}
	}

}

/*
FSavedPosition AShooterCharacter::GetPrecisePosition(float Time) {
	
	return this->SavedPositions;
}
*/

// End lag compensation code

void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Role == ROLE_Authority)
	{
		Health = GetMaxHealth();
		SpawnDefaultInventory();
	}

	// set initial mesh visibility (3rd person view)
	UpdatePawnMeshes();

	// create material instance for setting team colors (3rd person view)
	for (int32 iMat = 0; iMat < GetMesh()->GetNumMaterials(); iMat++)
	{
		MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(iMat));
	}

	// play respawn effects
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (RespawnFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, RespawnFX, GetActorLocation(), GetActorRotation());
		}

		if (RespawnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}
	}
}

void AShooterCharacter::Destroyed()
{
	Super::Destroyed();
	DestroyInventory();
}

void AShooterCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// switch mesh to 1st person view
	UpdatePawnMeshes();

	// reattach weapon if needed
	SetCurrentWeapon(CurrentWeapon);

	// set team colors for 1st person view
	UMaterialInstanceDynamic* Mesh1PMID = Mesh1P->CreateAndSetMaterialInstanceDynamic(0);
	UpdateTeamColors(Mesh1PMID);
}

void AShooterCharacter::PossessedBy(class AController* InController)
{
	Super::PossessedBy(InController);

	// [server] as soon as PlayerState is assigned, set team colors of this pawn for local player
	UpdateTeamColorsAllMIDs();
}

void AShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (PlayerState != NULL)
	{
		UpdateTeamColorsAllMIDs();
	}
}

FRotator AShooterCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool AShooterCharacter::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == Controller || TestPC == NULL)
	{
		return false;
	}

	AShooterPlayerState* TestPlayerState = Cast<AShooterPlayerState>(TestPC->PlayerState);
	AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);

	bool bIsEnemy = true;
	if (GetWorld()->GetGameState())
	{
		const AShooterGameMode* DefGame = GetWorld()->GetGameState()->GetDefaultGameMode<AShooterGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			bIsEnemy = DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}

	return bIsEnemy;
}

//////////////////////////////////////////////////////////////////////////
// Meshes

void AShooterCharacter::UpdatePawnMeshes()
{
	bool const bFirstPerson = IsFirstPerson();

	Mesh1P->MeshComponentUpdateFlag = !bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	Mesh1P->SetOwnerNoSee(!bFirstPerson);

	GetMesh()->MeshComponentUpdateFlag = bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetOwnerNoSee(bFirstPerson);
}

void AShooterCharacter::UpdateTeamColors(UMaterialInstanceDynamic* UseMID)
{
	if (UseMID)
	{
		AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);
		if (MyPlayerState != NULL)
		{
			float MaterialParam = (float)MyPlayerState->GetTeamNum();
			UseMID->SetScalarParameterValue(TEXT("Team Color Index"), MaterialParam);
		}
	}
}

void AShooterCharacter::OnCameraUpdate(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	USkeletalMeshComponent* DefMesh1P = Cast<USkeletalMeshComponent>(GetClass()->GetDefaultSubobjectByName(TEXT("PawnMesh1P")));
	const FMatrix DefMeshLS = FRotationTranslationMatrix(DefMesh1P->RelativeRotation, DefMesh1P->RelativeLocation);
	const FMatrix LocalToWorld = ActorToWorld().ToMatrixWithScale();

	// Mesh rotating code expect uniform scale in LocalToWorld matrix

	const FRotator RotCameraPitch(CameraRotation.Pitch, 0.0f, 0.0f);
	const FRotator RotCameraYaw(0.0f, CameraRotation.Yaw, 0.0f);

	const FMatrix LeveledCameraLS = FRotationTranslationMatrix(RotCameraYaw, CameraLocation) * LocalToWorld.Inverse();
	const FMatrix PitchedCameraLS = FRotationMatrix(RotCameraPitch) * LeveledCameraLS;
	const FMatrix MeshRelativeToCamera = DefMeshLS * LeveledCameraLS.Inverse();
	const FMatrix PitchedMesh = MeshRelativeToCamera * PitchedCameraLS;

	Mesh1P->SetRelativeLocationAndRotation(PitchedMesh.GetOrigin(), PitchedMesh.Rotator());
}


//////////////////////////////////////////////////////////////////////////
// Damage & death


void AShooterCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die(Health, FDamageEvent(dmgType.GetClass()), NULL, NULL);
}

void AShooterCharacter::Suicide()
{
	KilledBy(this);
}

void AShooterCharacter::KilledBy(APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasGodMode())
	{
		return 0.f;
	}

	if (Health <= 0.f)
	{
		return 0.f;
	}

	// Modify based on game rules.
	AShooterGameMode* const Game = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		Health -= ActualDamage;
		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return ActualDamage;
}


bool AShooterCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if (bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| Role != ROLE_Authority						// not authority
		|| GetWorld()->GetAuthGameMode<AShooterGameMode>() == NULL
		|| GetWorld()->GetAuthGameMode<AShooterGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}


bool AShooterCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AShooterGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AShooterCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}


void AShooterCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	bReplicateMovement = false;
	bTearOff = true;
	bIsDying = true;

	if (Role == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->KilledForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, false, "Damage");
			}
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// remove all weapons
	DestroyInventory();

	// switch back to 3rd person view
	UpdatePawnMeshes();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();

	if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	{
		LowHealthWarningPlayer->Stop();
	}

	if (RunLoopAC)
	{
		RunLoopAC->Stop();
	}

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	float DeathAnimDuration = PlayAnimMontage(DeathAnim);

	// Ragdoll
	if (DeathAnimDuration > 0.f)
	{
		// Trigger ragdoll a little before the animation early so the character doesn't
		// blend back to its normal position.
		const float TriggerRagdollTime = DeathAnimDuration - 0.7f;

		// Enable blend physics so the bones are properly blending against the montage.
		GetMesh()->bBlendPhysics = true;

		// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AShooterCharacter::SetRagdollPhysics, FMath::Max(0.1f, TriggerRagdollTime), false);
	}
	else
	{
		SetRagdollPhysics();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AShooterCharacter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, false, "Damage");
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	}

	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
	if (MyHUD)
	{
		MyHUD->NotifyWeaponHit(DamageTaken, DamageEvent, PawnInstigator);
	}

	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		AShooterPlayerController* InstigatorPC = Cast<AShooterPlayerController>(PawnInstigator->Controller);
		AShooterHUD* InstigatorHUD = InstigatorPC ? Cast<AShooterHUD>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}
}


void AShooterCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
	{
		SetLifeSpan(10.0f);
	}
}



void AShooterCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AShooterCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AShooterCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
}

//Pawn::PlayDying sets this lifespan, but when that function is called on client, dead pawn's role is still SimulatedProxy despite bTearOff being true. 
void AShooterCharacter::TornOff()
{
	SetLifeSpan(25.f);
}

bool AShooterCharacter::IsMoving()
{
	return FMath::Abs(GetLastMovementInputVector().Size()) > 0.f;
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void AShooterCharacter::SpawnDefaultInventory()
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	int32 NumWeaponClasses = DefaultInventoryClasses.Num();
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (DefaultInventoryClasses[i])
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AShooterWeapon* NewWeapon = GetWorld()->SpawnActor<AShooterWeapon>(DefaultInventoryClasses[i], SpawnInfo);
			AddWeapon(NewWeapon);
		}
	}

	// equip first weapon in inventory
	if (Inventory.Num() > 0)
	{
		EquipWeapon(Inventory[0]);
	}
}

void AShooterCharacter::DestroyInventory()
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	// remove all weapons from inventory and destroy them
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		AShooterWeapon* Weapon = Inventory[i];
		if (Weapon)
		{
			RemoveWeapon(Weapon);
			Weapon->Destroy();
		}
	}
}

void AShooterCharacter::AddWeapon(AShooterWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);
	}
}

void AShooterCharacter::RemoveWeapon(AShooterWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnLeaveInventory();
		Inventory.RemoveSingle(Weapon);
	}
}

AShooterWeapon* AShooterCharacter::FindWeapon(TSubclassOf<AShooterWeapon> WeaponClass)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(WeaponClass))
		{
			return Inventory[i];
		}
	}

	return NULL;
}

void AShooterCharacter::EquipWeapon(AShooterWeapon* Weapon)
{
	if (Weapon)
	{
		if (Role == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon, CurrentWeapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}

bool AShooterCharacter::ServerEquipWeapon_Validate(AShooterWeapon* Weapon)
{
	return true;
}

void AShooterCharacter::ServerEquipWeapon_Implementation(AShooterWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void AShooterCharacter::OnRep_CurrentWeapon(AShooterWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AShooterCharacter::SetCurrentWeapon(AShooterWeapon* NewWeapon, AShooterWeapon* LastWeapon)
{
	AShooterWeapon* LocalLastWeapon = NULL;

	if (LastWeapon != NULL)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!

		NewWeapon->OnEquip(LastWeapon);
	}
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AShooterCharacter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}

void AShooterCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

bool AShooterCharacter::CanFire() const
{
	return IsAlive();
}

bool AShooterCharacter::CanReload() const
{
	return true;
}

void AShooterCharacter::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;

	if (TargetingSound)
	{
		UGameplayStatics::SpawnSoundAttached(TargetingSound, GetRootComponent());
	}

	if (Role < ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool AShooterCharacter::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void AShooterCharacter::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

//////////////////////////////////////////////////////////////////////////
// Movement

void AShooterCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;

	if (Role < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}
}

bool AShooterCharacter::ServerSetRunning_Validate(bool bNewRunning, bool bToggle)
{
	return true;
}

void AShooterCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}

void AShooterCharacter::UpdateRunSounds()
{
	const bool bIsRunSoundPlaying = RunLoopAC != nullptr && RunLoopAC->IsActive();
	const bool bWantsRunSoundPlaying = IsRunning() && IsMoving();

	// Don't bother playing the sounds unless we're running and moving.
	if (!bIsRunSoundPlaying && bWantsRunSoundPlaying)
	{
		if (RunLoopAC != nullptr)
		{
			RunLoopAC->Play();
		}
		else if (RunLoopSound != nullptr)
		{
			RunLoopAC = UGameplayStatics::SpawnSoundAttached(RunLoopSound, GetRootComponent());
			if (RunLoopAC != nullptr)
			{
				RunLoopAC->bAutoDestroy = false;
			}
		}
	}
	else if (bIsRunSoundPlaying && !bWantsRunSoundPlaying)
	{
		RunLoopAC->Stop();
		if (RunStopSound != nullptr)
		{
			UGameplayStatics::SpawnSoundAttached(RunStopSound, GetRootComponent());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Animations

float AShooterCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

void AShooterCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance &&
		UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOut.GetBlendTime(), AnimMontage);
	}
}

void AShooterCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}


//////////////////////////////////////////////////////////////////////////
// Input

void AShooterCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &AShooterCharacter::MoveUp);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AShooterCharacter::OnStartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AShooterCharacter::OnStopFire);

	PlayerInputComponent->BindAction("Targeting", IE_Pressed, this, &AShooterCharacter::OnStartTargeting);
	PlayerInputComponent->BindAction("Targeting", IE_Released, this, &AShooterCharacter::OnStopTargeting);

	PlayerInputComponent->BindAction("NextWeapon", IE_Pressed, this, &AShooterCharacter::OnNextWeapon);
	PlayerInputComponent->BindAction("PrevWeapon", IE_Pressed, this, &AShooterCharacter::OnPrevWeapon);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AShooterCharacter::OnReload);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::OnStartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AShooterCharacter::OnStopJump);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AShooterCharacter::OnStartRunning);
	PlayerInputComponent->BindAction("RunToggle", IE_Pressed, this, &AShooterCharacter::OnStartRunningToggle);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &AShooterCharacter::OnStopRunning);
}


void AShooterCharacter::MoveForward(float Val)
{
	if (Controller && Val != 0.f)
	{
		// Limit pitch when walking or falling
		const bool bLimitRotation = (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling());
		const FRotator Rotation = bLimitRotation ? GetActorRotation() : Controller->GetControlRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
		AddMovementInput(Direction, Val);
	}
}

void AShooterCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		const FQuat Rotation = GetActorQuat();
		const FVector Direction = FQuatRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
		AddMovementInput(Direction, Val);
	}
}

void AShooterCharacter::MoveUp(float Val)
{
	if (Val != 0.f)
	{
		// Not when walking or falling.
		if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
		{
			return;
		}

		AddMovementInput(FVector::UpVector, Val);
	}
}

void AShooterCharacter::TurnAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Val * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::OnStartFire()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartWeaponFire();
	}
}

void AShooterCharacter::OnStopFire()
{
	StopWeaponFire();
}

void AShooterCharacter::OnStartTargeting()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		SetTargeting(true);
	}
}

void AShooterCharacter::OnStopTargeting()
{
	SetTargeting(false);
}

void AShooterCharacter::OnNextWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* NextWeapon = Inventory[(CurrentWeaponIdx + 1) % Inventory.Num()];
			EquipWeapon(NextWeapon);
		}
	}
}

void AShooterCharacter::OnPrevWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* PrevWeapon = Inventory[(CurrentWeaponIdx - 1 + Inventory.Num()) % Inventory.Num()];
			EquipWeapon(PrevWeapon);
		}
	}
}

void AShooterCharacter::OnReload()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}

void AShooterCharacter::OnStartRunning()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, false);
	}
}

void AShooterCharacter::OnStartRunningToggle()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, true);
	}
}

void AShooterCharacter::OnStopRunning()
{
	SetRunning(false, false);
}

bool AShooterCharacter::IsRunning() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}

	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().GetSafeNormal2D() | GetActorForwardVector()) > -0.1;
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bWantsToRunToggled && !IsRunning())
	{
		SetRunning(false, false);
	}
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasHealthRegen())
	{
		if (this->Health < this->GetMaxHealth())
		{
			this->Health += 5 * DeltaSeconds;
			if (Health > this->GetMaxHealth())
			{
				Health = this->GetMaxHealth();
			}
		}
	}

	if (GEngine->UseSound())
	{
		if (LowHealthSound)
		{
			if ((this->Health > 0 && this->Health < this->GetMaxHealth() * LowHealthPercentage) && (!LowHealthWarningPlayer || !LowHealthWarningPlayer->IsPlaying()))
			{
				LowHealthWarningPlayer = UGameplayStatics::SpawnSoundAttached(LowHealthSound, GetRootComponent(),
					NAME_None, FVector(ForceInit), EAttachLocation::KeepRelativeOffset, true);
				LowHealthWarningPlayer->SetVolumeMultiplier(0.0f);
			}
			else if ((this->Health > this->GetMaxHealth() * LowHealthPercentage || this->Health < 0) && LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
			{
				LowHealthWarningPlayer->Stop();
			}
			if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
			{
				const float MinVolume = 0.3f;
				const float VolumeMultiplier = (1.0f - (this->Health / (this->GetMaxHealth() * LowHealthPercentage)));
				LowHealthWarningPlayer->SetVolumeMultiplier(MinVolume + (1.0f - MinVolume) * VolumeMultiplier);
			}
		}

		UpdateRunSounds();
	}

	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bLocallyControlled = (PC ? PC->IsLocalController() : false);
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
	    USoundNodeLocalPlayer::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
	
	TArray<FVector> PointsToTest;
	BuildPauseReplicationCheckPoints(PointsToTest);

	if (NetVisualizeRelevancyTestPoints == 1)
	{
		for (FVector PointToTest : PointsToTest)
		{
			DrawDebugSphere(GetWorld(), PointToTest, 10.0f, 8, FColor::Red);
		}
	}

	// if server and you're drawing other people's hitboxes
	// can't have both server and client showing hitboxes at the same time. Otherwise you short circuit
	// and crash everything at runtime.
	if (HasAuthority() && !bLocallyControlled) {
		DrawSavedPositions(SavedPositions);
	}
}

void AShooterCharacter::BeginDestroy()
{
	Super::BeginDestroy();

	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			USoundNodeLocalPlayer::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void AShooterCharacter::OnStartJump()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		bPressedJump = true;
	}
}

void AShooterCharacter::OnStopJump()
{
	bPressedJump = false;
	StopJumping();
}

//////////////////////////////////////////////////////////////////////////
// Replication

void AShooterCharacter::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE(AShooterCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout);
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION(AShooterCharacter, Inventory, COND_OwnerOnly);

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(AShooterCharacter, bIsTargeting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AShooterCharacter, bWantsToRun, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(AShooterCharacter, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(AShooterCharacter, CurrentWeapon);
	DOREPLIFETIME(AShooterCharacter, Health);
}

bool AShooterCharacter::IsReplicationPausedForConnection(const FNetViewer& ConnectionOwnerNetViewer)
{
	if (NetEnablePauseRelevancy == 1)
	{
		APlayerController* PC = Cast<APlayerController>(ConnectionOwnerNetViewer.InViewer);
		check(PC);

		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

		FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(LineOfSight), true, PC->GetPawn());
		CollisionParams.AddIgnoredActor(this);

		TArray<FVector> PointsToTest;
		BuildPauseReplicationCheckPoints(PointsToTest);

		for (FVector PointToTest : PointsToTest)
		{
			if (!GetWorld()->LineTraceTestByChannel(PointToTest, ViewLocation, ECC_Visibility, CollisionParams))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void AShooterCharacter::OnReplicationPausedChanged(bool bIsReplicationPaused)
{
	GetMesh()->SetHiddenInGame(bIsReplicationPaused, true);
}

AShooterWeapon* AShooterCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

int32 AShooterCharacter::GetInventoryCount() const
{
	return Inventory.Num();
}

AShooterWeapon* AShooterCharacter::GetInventoryWeapon(int32 index) const
{
	return Inventory[index];
}

USkeletalMeshComponent* AShooterCharacter::GetPawnMesh() const
{
	return IsFirstPerson() ? Mesh1P : GetMesh();
}

USkeletalMeshComponent* AShooterCharacter::GetSpecifcPawnMesh(bool WantFirstPerson) const
{
	return WantFirstPerson == true ? Mesh1P : GetMesh();
}

FName AShooterCharacter::GetWeaponAttachPoint() const
{
	return WeaponAttachPoint;
}

float AShooterCharacter::GetTargetingSpeedModifier() const
{
	return TargetingSpeedModifier;
}

bool AShooterCharacter::IsTargeting() const
{
	return bIsTargeting;
}

float AShooterCharacter::GetRunningSpeedModifier() const
{
	return RunningSpeedModifier;
}

bool AShooterCharacter::IsFiring() const
{
	return bWantsToFire;
};

bool AShooterCharacter::IsFirstPerson() const
{
	return IsAlive() && Controller && Controller->IsLocalPlayerController();
}

int32 AShooterCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<AShooterCharacter>()->Health;
}

bool AShooterCharacter::IsAlive() const
{
	return Health > 0;
}

float AShooterCharacter::GetLowHealthPercentage() const
{
	return LowHealthPercentage;
}

void AShooterCharacter::UpdateTeamColorsAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdateTeamColors(MeshMIDs[i]);
	}
}

void AShooterCharacter::BuildPauseReplicationCheckPoints(TArray<FVector>& RelevancyCheckPoints)
{
	FBoxSphereBounds Bounds = GetCapsuleComponent()->CalcBounds(GetCapsuleComponent()->GetComponentTransform());
	FBox BoundingBox = Bounds.GetBox();
	float XDiff = Bounds.BoxExtent.X * 2;
	float YDiff = Bounds.BoxExtent.Y * 2;

	RelevancyCheckPoints.Add(BoundingBox.Min);
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X + XDiff, BoundingBox.Min.Y, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X, BoundingBox.Min.Y + YDiff, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X + XDiff, BoundingBox.Min.Y + YDiff, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X - XDiff, BoundingBox.Max.Y, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X, BoundingBox.Max.Y - YDiff, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X - XDiff, BoundingBox.Max.Y - YDiff, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(BoundingBox.Max);
}