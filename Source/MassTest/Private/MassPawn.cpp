
#include "MassPawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EntityCommon.h"
#include "MassCommonUtils.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassEntityUtils.h"
#include "MassSimulationSubsystem.h"

AMassPawn::AMassPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = true;
}

void AMassPawn::BeginPlay()
{
	Super::BeginPlay();

	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Purple, TEXT("BEGINPLAY"));

	check(GetMassEntityConfig());
	const FMassEntityTemplate& Template = GetMassEntityConfig()->GetOrCreateEntityTemplate(*GetWorld());
	FMassEntityManager& Manager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
	
	EntityHandle = Manager.CreateEntity(Template.GetArchetype(), Template.GetSharedFragmentValues());
	Manager.GetFragmentDataChecked<FActorHandleFragment>(EntityHandle).Actor = this;
	Manager.GetFragmentDataChecked<FCapsuleFragment>(EntityHandle).Radius = 34.f;
	Manager.GetFragmentDataChecked<FCapsuleFragment>(EntityHandle).HalfHeight = 88.f;
}

void AMassPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (EntityHandle.IsValid())
	{
		UE::Mass::Utils::GetEntityManagerChecked(*GetWorld()).Defer().DestroyEntity(EntityHandle);
		EntityHandle.Reset();
	}
}

void AMassPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//UE::Mass::Utils::GetEntityManagerChecked(*GetWorld()).GetFragmentDataChecked<FMovementInputFragment>(EntityHandle) = (FVector2f)CastChecked<UEnhancedInputComponent>(InputComponent)->GetBoundActionValue(GetMoveAction()).Get<FVector2D>();
}

void AMassPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalViewingPlayerController()->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);
	
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(GetInputMappingContext(), 0);

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EIC->BindActionValue(GetMoveAction());
	
	EIC->BindAction(GetMoveAction(), ETriggerEvent::Triggered, this, &AMassPawn::OnMove);
	EIC->BindAction(GetLookAction(), ETriggerEvent::Triggered, this, &AMassPawn::OnLook);
	EIC->BindAction(GetJumpAction(), ETriggerEvent::Triggered, this, &AMassPawn::OnJump);
}

void AMassPawn::OnMove(const FInputActionValue& Value)
{
	FVector2f Input = (FVector2f)Value.Get<FVector2D>();
	Input = Input.GetRotated(GetControlRotation().Yaw);
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, *CastChecked<UEnhancedInputComponent>(InputComponent)->GetBoundActionValue(GetMoveAction()).Get<FVector2D>().ToString());
}

void AMassPawn::OnLook(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, *Input.ToString());
	AddControllerPitchInput(-Input.Y);
	AddControllerYawInput(Input.X);
}

void AMassPawn::OnJump()
{
	UE::Mass::Utils::GetEntityManagerChecked(*GetWorld()).GetFragmentDataChecked<FVelocityFragment>(EntityHandle).Velocity.Z += 50.f;
}

