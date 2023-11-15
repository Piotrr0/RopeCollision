// Copyright Epic Games, Inc. All Rights Reserved.

#include "RopeCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "RopeInstance.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "RopeInstance.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// ARopeCharacter

ARopeCharacter::ARopeCharacter() :
	RopeLength(3000.f),
	CurrentRopeLength(0.f),
	CurrentRopePartLength(0.f)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ARopeCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ARopeCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARopeCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARopeCharacter::Look);

		EnhancedInputComponent->BindAction(FireRopeAction, ETriggerEvent::Started, this, &ARopeCharacter::FireRope);
		EnhancedInputComponent->BindAction(FireRopeAction, ETriggerEvent::Completed, this, &ARopeCharacter::ReleaseRope);
	}
}

void ARopeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckIfRopeBlockPoint();
	NoLongerAttachedToRope();
	UpdateRope();
	CalculateRemainingRope();
}

void ARopeCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		MovementDirection = ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X;

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ARopeCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

FTransform ARopeCharacter::GetSocketTransfrom(FName SocketName)
{
	FTransform SocketTransform{};
	USkeletalMeshComponent* SkeletalMeshComponent = GetMesh();

	if (SkeletalMeshComponent->DoesSocketExist(SocketName))
	{
		SocketTransform = SkeletalMeshComponent->GetSocketTransform(SocketName);
	}
	return SocketTransform;
}

FVector ARopeCharacter::GetRopeLocation()
{
	return RopePoints.Last();
}

void ARopeCharacter::FireRope()
{
	#define LENGTH 10'000.f

	FVector SocketLocation = GetSocketTransfrom(FName("RopeSocket")).GetLocation(); // Start of the line trace
	FRotator AimRotation = GetBaseAimRotation(); // Get the aim rotation
	FVector EndLocation = UKismetMathLibrary::GetForwardVector(AimRotation)* LENGTH; // Get forward vector based of the character aim rotation

	//DrawDebugLine(GetWorld(), SocketLocation, EndLocation, FColor::Red, false, 1.f, 0.f, 1.f);
	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, SocketLocation, EndLocation, ECollisionChannel::ECC_Visibility);

	bRopeHit = bHit;
	if (HitResult.bBlockingHit)
	{
		if ((HitResult.ImpactPoint - SocketLocation).Size() > RopeLength)
		{
			bRopeHit = false;
			return;
		}


		auto RopeRef = Cast<ARopeInstance>(SpawnRope());
		if (RopeRef)
		{
			UE_LOG(LogTemp, Warning, TEXT("RopeRefSpawned"));
			RopeParts.Add(RopeRef);
		}

		HitLocation = HitResult.ImpactPoint;

		RopePoints.AddUnique(HitResult.ImpactPoint);
		RopePointsNormals.Add(HitResult.ImpactNormal);
		RopePointsTangents.Add(FVector::ZeroVector);

		return;
	}
	else
	{
		HitResult.TraceEnd;
	}
}

void ARopeCharacter::ReleaseRope()
{
	bRopeHit = false;
	RopePoints.Empty();
	RopePointsNormals.Empty();
	RopePointsTangents.Empty();

	for (AActor* Element : RopeParts)
	{
		if (Element)
		{
			Element->Destroy();
		}
	}
	RopeParts.Empty();
	RopePartsLengths.Empty();

	CurrentRopeLength = 0.f;
	CurrentRopePartLength = 0.f;

	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
}

AActor* ARopeCharacter::SpawnRope()
{
	FTransform BaseTransform = FTransform(FQuat::Identity, FVector::ZeroVector, FVector(1, 1, 1));
	auto NewRope = UGameplayStatics::BeginDeferredActorSpawnFromClass(GetWorld(), Rope, BaseTransform);
	if (NewRope)
	{
		return UGameplayStatics::FinishSpawningActor(NewRope, BaseTransform);
	}
	return nullptr;
}

void ARopeCharacter::CheckIfRopeBlockPoint()
{
	if (bRopeHit)
	{
		FVector SocketLocation = GetSocketTransfrom(FName("RopeSocket")).GetLocation();
		FVector End = UKismetMathLibrary::VLerp(SocketLocation, GetRopeLocation(), 0.99f);


		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, SocketLocation, End, ECollisionChannel::ECC_Visibility);
		if (HitResult.bBlockingHit)
		{
			FHitResult SecondHitResult;
			GetWorld()->LineTraceSingleByChannel(SecondHitResult, GetRopeLocation(), SocketLocation, ECollisionChannel::ECC_Visibility);

			FVector RopePointOffset = UKismetMathLibrary::VLerp(HitResult.ImpactNormal, RopePointsNormals.Last(), 0.5f);
			FVector HitPoint = (HitResult.ImpactPoint + RopePointOffset);

			FVector Normal = UKismetMathLibrary::VLerp(HitResult.Normal, SecondHitResult.Normal, 0.5f).GetSafeNormal(0.0001f);
			FVector Tangent = FVector::CrossProduct(HitResult.ImpactNormal, SecondHitResult.ImpactNormal);

			RopePoints.AddUnique(HitPoint);
			RopePointsNormals.Add(Normal);
			RopePointsTangents.Add(Tangent);

			HandleNewBlockPoint();
		}
	}
}

void ARopeCharacter::NoLongerAttachedToRope()
{
	if (RopePoints.Num() > 1 && bRopeHit)
	{
		FVector SocketLocation = GetSocketTransfrom(FName("RopeSocket")).GetLocation();
		int32 PreLastIndex = RopePoints.Num() - 2;
		FVector PreLastElement = RopePoints[PreLastIndex];

		FVector End = UKismetMathLibrary::VLerp(SocketLocation, PreLastElement, 0.99f);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, SocketLocation, End, ECollisionChannel::ECC_Visibility);
		if (!HitResult.bBlockingHit)
		{
			int32 index = RopePoints.Num() - 1;

			RopePoints.RemoveAt(index);
			RopePointsNormals.RemoveAt(index);
			RopePointsTangents.RemoveAt(index);

			FreeRopePart();
		}
	}
}

void ARopeCharacter::HandleNewBlockPoint()
{
	float Dist = FVector::Distance(RopePoints[RopePoints.Num() - 2], RopePoints.Last());
	CurrentRopePartLength += Dist;
	CurrentRopeLength += CurrentRopePartLength;

	RopePartsLengths.Add(Dist);

	auto* RopeRef = Cast<ARopeInstance>(SpawnRope());
	RopeParts.Add(RopeRef);
}

void ARopeCharacter::FreeRopePart()
{
	if (RopeParts.Last())
	{
		RopeParts.Last()->Destroy();
		RopeParts.RemoveAt(RopeParts.Num()-1);
	}
	CurrentRopeLength -= RopePartsLengths.Last();
	CurrentRopePartLength -= RopePartsLengths.Last();


	RopePartsLengths.RemoveAt(RopePartsLengths.Num() - 1);

}

void ARopeCharacter::UpdateRope()
{
	if (!bRopeHit) return;

	TArray<FVector> Locations = RopePoints;
	Locations.Add(GetSocketTransfrom(FName("RopeSocket")).GetLocation());

	for (size_t i = 0; i <= Locations.Num() - 2; i++)
	{
		if (RopeParts[i])
		{
			RopeParts[i]->SetRopeTransfrom(Locations[i], Locations[i+1]);
		}
	}
}

void ARopeCharacter::CalculateRemainingRope()
{
	if (RopePoints.Num() > 0)
	{
		FVector RopeSocketLocation = GetSocketTransfrom(FName("RopeSocket")).GetLocation();
		float CurrentDistanceToRopeEnd = FVector::Distance(RopeSocketLocation, RopePoints.Last());
		CurrentRopeLength = CurrentDistanceToRopeEnd + CurrentRopePartLength;

		if (CurrentRopeLength > RopeLength)
		{
			FVector CharacterPosition = RopeSocketLocation;
			FVector LastRopePoint = RopePoints.Last();

			FVector DirectionToCharacter = (CharacterPosition - LastRopePoint).GetSafeNormal();

			float DotProduct = FVector::DotProduct(DirectionToCharacter, MovementDirection.GetSafeNormal());
			if (DotProduct >= 0)
			{
				GetCharacterMovement()->MaxWalkSpeed = 0.f;
				GetCharacterMovement()->MinAnalogWalkSpeed = 0.f;
			}
			else
			{
				GetCharacterMovement()->MaxWalkSpeed = 500.f;
				GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
			}
		}
	}
}

