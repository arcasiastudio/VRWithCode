// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Classes/Materials/MaterialInstanceDynamic.h"
#include "VRCharacter.generated.h"

UCLASS()
class HTCVIVE_EXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//Refrence Method
private:
	//Refactoring update navigation
	bool FindTeleportDestination(TArray<FVector> &OutPath, FVector &OutLocation);
	//
	void UpdateDestinationMarker();
	// Set Blinkers to dynamic using curves
	void UpdateBlinkers();
	//Object pool at teleport parabolic
	void DrawTeleportPath(const TArray<FVector>& Path);
	//Set list of spline
	void UpdateSpline(const TArray<FVector>& Path);
	//Set Center of screen dynamicly
	FVector2D GetBlinkerCenter();

	void MoveForward(float throttle);
	void MoveRight(float throttle);
	void BeginTeleport();
	void FinishTeleport();
	void StartFade(float FromAlpha, float ToAlpha);

	//Refrence Class to Child Object/Actor in viewport, spawn actor at runtime
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	class UCameraComponent* Camera;
	
	UPROPERTY(EditAnywhere)
	class UMotionControllerComponent* LeftController;

	UPROPERTY(EditAnywhere)
	class UMotionControllerComponent* RightController;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;

	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY()
	class UPostProcessComponent* PostProcessing;

	UPROPERTY()
	class UMaterialInstanceDynamic* BlinkerMaterialInstance;

	//this below is for instance / spawn component at run time
	UPROPERTY(VisibleAnywhere)
	//TArray<class UStaticMeshComponent*> TeleportPathMeshPool;
	TArray<class USplineMeshComponent*> TeleportPathMeshPool;


	//Refrence Variable & Config to add component in parent object
private:
	UPROPERTY(EditAnywhere)
		float MaxTeleportDistance = 1000;

	UPROPERTY(EditAnywhere)
		float TeleportProjectileRadius = 10;

	UPROPERTY(EditAnywhere)
		float TeleportProjectileSpeed = 800;

	UPROPERTY(EditAnywhere)
		float TeleportSimulationTime = 2;

	UPROPERTY(EditAnywhere)
		float TeleportFadeTime = 1;

	UPROPERTY(EditAnywhere)
		FVector TeleportProjectionExtent = FVector(100, 100, 100); //1m x 1m x 1m
	
	UPROPERTY(EditAnywhere)
		class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY(EditAnywhere)
		class UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditDefaultsOnly)
		class UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly)
		class UMaterialInterface* TeleportArchMaterial;
};
