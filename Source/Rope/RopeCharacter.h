// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RopeInstance.h"
#include "InputActionValue.h"
#include "RopeCharacter.generated.h"


UCLASS(config=Game)
class ARopeCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FireRopeAction;


public:
	ARopeCharacter();
	

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	FTransform GetSocketTransfrom(FName SocketName);

	FVector GetRopeLocation();

	void FireRope();

	void ReleaseRope();

	AActor* SpawnRope();
		
	void CheckIfRopeBlockPoint();

	void NoLongerAttachedToRope();

	void HandleNewBlockPoint();

	void FreeRopePart();

	void UpdateRope();

	void CalculateRemainingRope();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	FVector MovementDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	FVector HitLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	bool bRopeHit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> RopePoints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> RopePointsNormals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> RopePointsTangents;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ARopeInstance> Rope;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	TArray<ARopeInstance*> RopeParts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	TArray<float> RopePartsLengths;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	float RopeLength;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	float CurrentRopePartLength;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	float CurrentRopeLength;




public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

