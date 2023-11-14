// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RopeInstance.generated.h"

UCLASS()
class ROPE_API ARopeInstance : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARopeInstance();

;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetRopeTransfrom(FVector StartLocation, FVector EndLocation);

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rope, meta = (AllowPrivateAccess = "true"))
	class USplineMeshComponent* Rope;

};
