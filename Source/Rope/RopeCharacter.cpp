// Copyright Epic Games, Inc. All Rights Reserved.

#include "RopeCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SplineMeshComponent.h"

//////////////////////////////////////////////////////////////////////////
// ARopeCharacter

ARopeCharacter::ARopeCharacter()
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

	Rope = CreateDefaultSubobject<USplineMeshComponent>(TEXT("Rope"));

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

	UpdateHookLocation();
	NoLongerAttachedToRope();
	SpawnRope();
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

}

void ARopeCharacter::SpawnRope()
{
	//Spawn Rope
}

void ARopeCharacter::UpdateHookLocation()
{
	if (bRopeHit)
	{
		FVector SocketLocation = GetSocketTransfrom(FName("RopeSocket")).GetLocation();
		FVector End = UKismetMathLibrary::VLerp(SocketLocation, GetRopeLocation(), 0.99f);

		//DrawDebugLine(GetWorld(), SocketLocation, End, FColor::Red, false, 1.f, 0, 1.0f);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, SocketLocation, End, ECollisionChannel::ECC_Visibility);
		if (HitResult.bBlockingHit)
		{
			FHitResult SecondHitResult;
			//DrawDebugLine(GetWorld(), GetRopeLocation(), SocketLocation, FColor::Blue, false, 2.f, 0, 3.0f);
			GetWorld()->LineTraceSingleByChannel(SecondHitResult, GetRopeLocation(), SocketLocation, ECollisionChannel::ECC_Visibility);

			FVector RopePointOffset = UKismetMathLibrary::VLerp(HitResult.ImpactNormal, RopePointsNormals.Last(), 0.5f);
			FVector HitPoint = (HitResult.ImpactPoint + RopePointOffset);

			FVector Normal = UKismetMathLibrary::VLerp(HitResult.Normal, SecondHitResult.Normal, 0.5f).GetSafeNormal(0.0001f);
			FVector Tangent = FVector::CrossProduct(HitResult.ImpactNormal, SecondHitResult.ImpactNormal);

			RopePoints.AddUnique(HitPoint);
			RopePointsNormals.Add(Normal);
			RopePointsTangents.Add(Tangent);
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
			RopePoints.RemoveAt(RopePoints.Num() - 1);
			RopePointsNormals.RemoveAt(RopePointsNormals.Num() - 1);
			RopePointsTangents.RemoveAt(RopePointsTangents.Num() - 1);
		}
	}
}




