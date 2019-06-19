// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Classes/Components/StaticMeshComponent.h"
#include "Public/TimerManager.h"
#include "GameFramework/Character.h"
#include "Classes/Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem/Public/NavigationSystem.h"
#include "Classes/Components/PostProcessComponent.h"
#include "Classes/Materials/MaterialInstanceDynamic.h"
#include "Classes/Curves/CurveFloat.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Classes/Components/SplineComponent.h"
#include "Classes/Components/SplineMeshComponent.h"


// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(RightController);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessing = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessingComponent"));
	PostProcessing->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	DestinationMarker->SetVisibility(false);

	if (BlinkerMaterialBase != nullptr)
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessing->AddOrUpdateBlendable(BlinkerMaterialInstance);

		//fungsi ini di pindah ke fungsi update blinker
		//BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), 0.2);
	}
	
	

}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*
	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);
	*/
	

	UpdateDestinationMarker();
	UpdateBlinkers();
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);

	//Teleport
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);

}

bool AVRCharacter::FindTeleportDestination(TArray<FVector>& OutPath, FVector& OutLocation)
{
	FVector Start = RightController->GetComponentLocation();
	FVector Look = RightController->GetForwardVector();

	//this method works on camera raycasting
	//Look = Look.RotateAngleAxis(30, RightController->GetRightVector());
	//FVector End = Start + Look * MaxTeleportDistance;
	//FHitResult HitResult;
	//bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
	//if (!bHit) return false;

	//this method works for hand controller raycasting
	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,
		Start,
		Look * TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this
	);
	//this line of code below is not necessary, just for debug only
	//Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	//Params.bTraceComplex = true;

	FPredictProjectilePathResult Result;

	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);
	if (!bHit) return false;

	for (FPredictProjectilePathPointData PointData : Result.PathData)
	{
		OutPath.Add(PointData.Location);
	}

	//Projecting to navigation
	FNavLocation NavLocation;
	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(this);
	bool bOnNavMesh = NavSystem->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent);
	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return true;
}

void AVRCharacter::UpdateDestinationMarker()
{
	TArray<FVector> Path;
	FVector Location;
	bool bHasDestination = FindTeleportDestination(Path, Location);

	if (bHasDestination)
	{
		if (!DestinationMarker->IsVisible())
		{
			DestinationMarker->SetVisibility(true);
		}

		DestinationMarker->SetWorldLocation(/*HitResult.Location*/ Location);

		//draw parabolic with spline
		//UpdateSpline(Path); 

		//draw parabolic with actual mesh
		DrawTeleportPath(Path);
	}
	else
	{
		DestinationMarker->SetVisibility(false);

		TArray<FVector> EmptyPath;
		DrawTeleportPath(EmptyPath);
	}
}

void AVRCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity != nullptr)
	{
		float Speed = GetVelocity().Size();
		float Radius = RadiusVsVelocity->GetFloatValue(Speed);

		BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

		FVector2D Center = GetBlinkerCenter();
		BlinkerMaterialInstance->SetVectorParameterValue(TEXT("CenterScreen"), FLinearColor(Center.X, Center.Y, 0));
	}
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector> &Path)
{
	UpdateSpline(Path);

	//set visiblity
	for (USplineMeshComponent* DynamicMesh : TeleportPathMeshPool)
	{
		DynamicMesh->SetVisibility(false);
	}

	int32 SegmentNumber = Path.Num() - 1;
	for (int32 i = 0; i < SegmentNumber; ++i)
	{
		
		// memory management, we want instance mesh only when we need it
		if (TeleportPathMeshPool.Num() <= i)
		{
			//instance new actor
			USplineMeshComponent* DynamicMesh = NewObject<USplineMeshComponent>(this);
			//by default spline mesh component is static comp, turn it to moveable comp
			DynamicMesh->SetMobility(EComponentMobility::Movable);
			//attach the actor to something in the scene
			DynamicMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			//Setting properties & component the actor that you want to spawn at runtime
			DynamicMesh->SetStaticMesh(TeleportArchMesh);
			DynamicMesh->SetMaterial(0, TeleportArchMaterial);
			//register actor
			DynamicMesh->RegisterComponent();
			//Add to List
			TeleportPathMeshPool.Add(DynamicMesh);
		}

		//instance new actor
		USplineMeshComponent* DynamicMesh = TeleportPathMeshPool[i];
		//set visibel when actually using it
		DynamicMesh->SetVisibility(true);

		//Variable to execute local pos spline mesh
		FVector StartPos, StartTangent, EndPos, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		//set to end segment and position too
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent);
		//Set Spline mesh location
		DynamicMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		//Set Actor location
		//DynamicMesh->SetWorldLocation(Path[i]);
	}
}

void AVRCharacter::UpdateSpline(const TArray<FVector> &Path)
{
	TeleportPath->ClearSplinePoints(false);
	for (int32 i = 0; i < Path.Num(); ++i)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		//FSplinePoint Point(i, Path[i], ESplinePointType::Curve);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}

	TeleportPath->UpdateSpline(); //ini method dari sono nya
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		return FVector2D(0.5, 0.5);
	}

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}

	//Casting player controller
	APlayerController* PlayerCtrl = Cast<APlayerController>(GetController());

	if (PlayerCtrl == nullptr)
	{
		return FVector2D(0.5, 0.5);
	}

	//Project world location to playercontroller
	FVector2D ScreenStationaryLocation;
	PlayerCtrl->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);

	//implement to screen
	int32 SizeX, SizeY;
	PlayerCtrl->GetViewportSize(SizeX, SizeY);
	//Update screen location into UV Material Blink
	ScreenStationaryLocation.X /= SizeX; //To devided normalized
	ScreenStationaryLocation.Y /= SizeY;

	return ScreenStationaryLocation;
}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}

void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

void AVRCharacter::BeginTeleport()
{
	StartFade(0, 1);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime, false);

}

void AVRCharacter::FinishTeleport()
{
	//Biar ga ngblink pas teleport
	//GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	//Pindah tempat
	//SetActorLocation(DestinationMarker->GetComponentLocation());

	//SetActorLocation(DestinationMarker->GetComponentLocation() + GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	FVector Destination = DestinationMarker->GetComponentLocation();
	Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(Destination);

	StartFade(1, 0);
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha)
{ 
	APlayerController* PlayerCtrl = Cast<APlayerController>(GetController());

	if (PlayerCtrl != nullptr)
	{
		PlayerCtrl->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}

