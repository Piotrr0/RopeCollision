// Fill out your copyright notice in the Description page of Project Settings.


#include "RopeInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SplineMeshComponent.h"

// Sets default values
ARopeInstance::ARopeInstance()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Rope = CreateDefaultSubobject<USplineMeshComponent>(TEXT("Rope"));
	SetRootComponent(Rope);
}

void ARopeInstance::SetRopeTransfrom(FVector StartLocation, FVector EndLocation)
{
	float Distance = UKismetMathLibrary::Vector_Distance(StartLocation, EndLocation);
	SetActorLocation(StartLocation);
	SetActorScale3D(FVector((Distance / 100.f), 0.1f, 0.1f));
	SetActorRotation(UKismetMathLibrary::FindLookAtRotation(StartLocation, EndLocation), ETeleportType::TeleportPhysics);
}

// Called when the game starts or when spawned
void ARopeInstance::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARopeInstance::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

