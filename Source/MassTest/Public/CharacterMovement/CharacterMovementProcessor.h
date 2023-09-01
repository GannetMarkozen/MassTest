
#pragma once

#include "EnhancedInputComponent.h"
#include "EntityCommon.h"
#include "MassProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntity/Private/MassArchetypeData.h"
#include "CharacterMovementProcessor.generated.h"

UCLASS()
class MASSTEST_API UInputVelocityProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	explicit UInputVelocityProcessor();

protected:
	UPROPERTY()
	UInputAction* MoveAction;
	
	//~ Begin UMassProcessor interface
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	//~ End UMassProcessor interface

private:
	FMassEntityQuery InputQuery;
};

inline UInputVelocityProcessor::UInputVelocityProcessor()
{
	bRequiresGameThreadExecution = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::Standalone | (int32)EProcessorExecutionFlags::Client;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::ProcessInput;

	static const ConstructorHelpers::FObjectFinder<UInputAction> MoveForwardActionFinder{TEXT("/Game/Input/IA_Move.IA_Move")};
	MoveAction = MoveForwardActionFinder.Object;
}

inline void UInputVelocityProcessor::ConfigureQueries()
{
	InputQuery.AddRequirement<FMovementInputFragment>(EMassFragmentAccess::ReadWrite);
	InputQuery.RegisterWithProcessor(*this);
}

inline void UInputVelocityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	const APawn* Pawn = LIKELY(LocalPlayer) ? LocalPlayer->PlayerController->GetPawn() : nullptr;
	if (UNLIKELY(!Pawn)) return;

	FVector2f Value = (FVector2f)CastChecked<UEnhancedInputComponent>(Pawn->InputComponent)->GetBoundActionValue(MoveAction).Get<FVector2D>();
	Value.Normalize();
	
	InputQuery.ForEachEntityChunk(EntityManager, Context, [this, &Value](FMassExecutionContext& Context) -> void
	{
		const TArrayView<FMovementInputFragment> MovementInputs = Context.GetMutableFragmentView<FMovementInputFragment>();

		for (int32 i = 0; i < MovementInputs.Num(); ++i)
		{
			MovementInputs[i] = Value;
		}
	});
}






UCLASS()
class MASSTEST_API UCharacterMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	explicit UCharacterMovementProcessor();

protected:
	void PerformMovement(const FMassExecutionContext& Context, FTransform& InOutTransform, FVector3f& InOutVelocity, const float Radius, const float HalfHeight, const bool bDrawDebug = false) const;
	
	//~ Begin UMassProcessor interface
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	//~ End UMassProcessor interface

	static constexpr float MOVE_VELOCITY = 2500.f;
	static constexpr float MAX_MOVE_SPEED = 900.f;
	static constexpr float MAX_FALL_SPEED = 1500.f;
	static constexpr float GROUND_FRICTION = 2000.f;
	static constexpr uint8 MAX_SWEEP_BOUNCES = 5;

private:
	FMassEntityQuery GroundedCharacterQuery;
};

inline UCharacterMovementProcessor::UCharacterMovementProcessor()
{
	bRequiresGameThreadExecution = false;
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::ProcessInput);
}

inline void UCharacterMovementProcessor::ConfigureQueries()
{
	GroundedCharacterQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	GroundedCharacterQuery.AddRequirement<FVelocityFragment>(EMassFragmentAccess::ReadWrite);
	GroundedCharacterQuery.AddRequirement<FCapsuleFragment>(EMassFragmentAccess::ReadOnly);
	GroundedCharacterQuery.AddRequirement<FMovementInputFragment>(EMassFragmentAccess::ReadOnly);
	GroundedCharacterQuery.AddTagRequirement<FCharacterMovementTag>(EMassFragmentPresence::All);
	GroundedCharacterQuery.AddTagRequirement<FGroundedMovementTag>(EMassFragmentPresence::All);
	GroundedCharacterQuery.AddTagRequirement<FGravityTag>(EMassFragmentPresence::Optional);
	GroundedCharacterQuery.RegisterWithProcessor(*this);
}

static TAutoConsoleVariable<bool> CVarMassTestDebugCMC{
	TEXT("MassTest.DebugCMC"),
	false,
	TEXT("")};

inline void UCharacterMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UCharacterMovementProcessor::Execute"), STAT_CharacterMovementProcessor, STATGROUP_MassTest);

	GroundedCharacterQuery.ForEachEntityChunk(EntityManager, Context, [&](FMassExecutionContext& Context) -> void
	{
		const TArrayView<FTransformFragment> Transforms = Context.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FVelocityFragment> Velocities = Context.GetMutableFragmentView<FVelocityFragment>();
		const TConstArrayView<FCapsuleFragment> Capsules = Context.GetFragmentView<FCapsuleFragment>();
		const TConstArrayView<FMovementInputFragment> MovementInputs = Context.GetFragmentView<FMovementInputFragment>();
		const bool bApplyGravity = Context.DoesArchetypeHaveTag<FGravityTag>();

		const float GravityZ = GetWorld()->GetGravityZ();

		for (int32 i = 0; i < Context.GetNumEntities(); ++i)
		{
			FTransform& RESTRICT Transform = Transforms[i].GetMutableTransform();
			FVector3f& RESTRICT Velocity = Velocities[i].Velocity;
			const FCapsuleFragment& RESTRICT Capsule = Capsules[i];

			//~ Apply gravity.
			Velocity.Z += GravityZ * Context.GetDeltaTimeSeconds() * bApplyGravity;
			//~

			//~ Apply lateral damping
			const FVector2D DampenedVelocity2D = FMath::Vector2DInterpConstantTo(FVector2D{Velocity.X, Velocity.Y}, FVector2D::ZeroVector, Context.GetDeltaTimeSeconds(), GROUND_FRICTION);
			Velocity.X = DampenedVelocity2D.X;
			Velocity.Y = DampenedVelocity2D.Y;
			//~

			//~ Add movement input to velocity.
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, FString::Printf(TEXT("MovementInput == %s"), *MovementInputs[i].MovementInput.ToString()));
			const FVector2f& RESTRICT MovementInput = MovementInputs[i].MovementInput;

			FQuat YawRotation = Transform.GetRotation();
			YawRotation = FQuat{0.0, 0.0, YawRotation.Z, YawRotation.W}.GetNormalized();
			const FVector3f AddVelocity = (FVector3f)YawRotation.RotateVector(FVector{MovementInput.X, MovementInput.Y, 0.0}) * MOVE_VELOCITY;
			
			FVector3f NewVelocity = Velocity + AddVelocity * Context.GetDeltaTimeSeconds();

			const FVector3f AddedVelocity = NewVelocity - Velocity;
			Velocity = NewVelocity;
			//~

			//~ Clamp movement lateral velocity.
			if (Velocity.SizeSquared2D() > FMath::Square(MAX_MOVE_SPEED) && (AddedVelocity.X * Velocity.X + AddedVelocity.Y * Velocity.Y) > 0.f)
			{
				const float NewVelocitySize = Velocity.Size2D();
				const FVector3f NewVelocityDirection = FVector3f{Velocity.X, Velocity.Y, 0.f} / NewVelocitySize;
				const float ProjectedAddVelocity = AddedVelocity | NewVelocityDirection;
				Velocity -= NewVelocityDirection * FMath::Min(ProjectedAddVelocity, NewVelocitySize - MAX_MOVE_SPEED);
			}
			//~

#if 0
			//~ Perform movement.
			uint8 NumSweepBounces = 0;
			FVector ProjectedLocation = Transform.GetLocation() + Velocity * Context.GetDeltaTimeSeconds();

			do
			{
			  FHitResult HitResult;
			  if (!GetWorld()->SweepSingleByChannel(HitResult, Transform.GetLocation(), ProjectedLocation, Transform.GetRotation(), ECC_WorldStatic, FCollisionShape::MakeCapsule(Capsule.Radius, Capsule.HalfHeight))) break;
			
			  const double TranslationProjection = (ProjectedLocation - HitResult.Location) | HitResult.Normal;
			  if (TranslationProjection >= 0.f) break;
			
			  ProjectedLocation -= HitResult.Normal * (TranslationProjection - UE_DOUBLE_KINDA_SMALL_NUMBER);

			  const float VelocityProjection = Velocity | HitResult.Normal;
			  Velocity -= HitResult.Normal * VelocityProjection;
			} while (++NumSweepBounces < MAX_SWEEP_BOUNCES);
			//~

			Transform.SetLocation(ProjectedLocation);
#endif

#if UE_BUILD_DEVELOPMENT
			FTransform TmpTransform = Transform;
			FVector3f TmpVelocity = (FVector3f)Transform.GetUnitAxis(EAxis::X) * 50000.f;
			PerformMovement(Context, TmpTransform, TmpVelocity, Capsule.Radius, Capsule.HalfHeight, true);
#endif

			PerformMovement(Context, Transform, Velocity, Capsule.Radius, Capsule.HalfHeight, false);
		}
	});
}

inline void UCharacterMovementProcessor::PerformMovement(const FMassExecutionContext& Context, FTransform& InOutTransform, FVector3f& InOutVelocity, const float Radius, const float HalfHeight, const bool bDrawDebug) const
{
	uint8 NumSweepBounces = 0;
	FVector CurrentLocation = InOutTransform.GetLocation();
	FVector ProjectedLocation = InOutTransform.GetLocation() + InOutVelocity * Context.GetDeltaTimeSeconds();
	
	const FCollisionShape CapsuleCollision = FCollisionShape::MakeCapsule(Radius, HalfHeight);

#if UE_BUILD_DEVELOPMENT
	if (bDrawDebug)
	{
		DrawDebugCapsule(GetWorld(), ProjectedLocation, HalfHeight, Radius, InOutTransform.GetRotation(), FColor::Orange, false, -1.f, 0, 1.f);
		DrawDebugLine(GetWorld(), InOutTransform.GetLocation(), ProjectedLocation, FColor::Orange, false, -1.f, 0, 1.f);
	}
#endif

#if 0
	const FVector3f VelocityDirection = InOutVelocity.GetSafeNormal();
	do
	{
		FHitResult HitResult;
		if (!GetWorld()->SweepSingleByChannel(HitResult, InOutTransform.GetLocation(), ProjectedLocation, InOutTransform.GetRotation(), ECC_WorldStatic, FCollisionShape::MakeCapsule(Radius, HalfHeight))) break;
			
		double TranslationProjection = (ProjectedLocation - HitResult.Location) | HitResult.Normal;
		if (TranslationProjection >= 0.f) break;

		#if 0
		FVector NewProjectedLocation = ProjectedLocation - HitResult.Normal * (TranslationProjection - UE_DOUBLE_KINDA_SMALL_NUMBER);
		for (uint8 i = 0; i < NumSweepBounces; ++i)
		{
			
		}
#endif
		
		FVector NewProjectedLocation = ProjectedLocation - HitResult.Normal * (TranslationProjection - UE_DOUBLE_KINDA_SMALL_NUMBER);
		


#if UE_BUILD_DEVELOPMENT
		if (bDrawDebug)
		{
			DrawDebugCapsule(GetWorld(), HitResult.Location, HalfHeight, Radius, InOutTransform.GetRotation(), FColor::Red, false, -1.f, 0, 1.f);
			DrawDebugCapsule(GetWorld(), NewProjectedLocation, HalfHeight, Radius, InOutTransform.GetRotation(), FColor::Yellow, false, -1.f, 0, 1.f);
			DrawDebugLine(GetWorld(), HitResult.Location, NewProjectedLocation, FColor::Yellow, false, -1.f, 0, 1.f);
		}
#endif
		
		ProjectedLocation = NewProjectedLocation;

		const float VelocityProjection = InOutVelocity | HitResult.Normal;
		InOutVelocity -= HitResult.Normal * VelocityProjection;
		
		PlaneHits[NumSweepBounces][0] = HitResult.Location;
		PlaneHits[NumSweepBounces][1] = HitResult.Normal;
	} while (++NumSweepBounces < MAX_SWEEP_BOUNCES);
#endif

	do
	{
		FHitResult Hit;
		if (!GetWorld()->SweepSingleByChannel(Hit, CurrentLocation, ProjectedLocation, InOutTransform.GetRotation(), ECC_WorldStatic, CapsuleCollision))
		{
			CurrentLocation = ProjectedLocation;
			if (bDrawDebug) DrawDebugCapsule(GetWorld(), CurrentLocation, HalfHeight, Radius, InOutTransform.GetRotation(), FColor::Green);
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, FString::Printf(TEXT("Early outting at %i!"), NumSweepBounces));
			break;
		}

		CurrentLocation = Hit.Location + Hit.Normal * UE_DOUBLE_KINDA_SMALL_NUMBER;

		if ((InOutVelocity | Hit.Normal) < 0.f)
		{
			InOutVelocity -= InOutVelocity.ProjectOnToNormal((FVector3f)Hit.Normal);
			ProjectedLocation -= (ProjectedLocation - Hit.Location).ProjectOnToNormal(Hit.Normal) + Hit.Normal * UE_DOUBLE_KINDA_SMALL_NUMBER;
		}
		
		if (bDrawDebug)
		{
			DrawDebugCapsule(GetWorld(), CurrentLocation, HalfHeight, Radius, InOutTransform.GetRotation(), FColor::Red, false, -1.f, 0, 1.f);
			DrawDebugCapsule(GetWorld(), ProjectedLocation, HalfHeight, Radius, InOutTransform.GetRotation(), FColor::Orange, false, -1.f, 0, 1.f);
			DrawDebugLine(GetWorld(), CurrentLocation, ProjectedLocation, FColor::Orange, false, -1.f, 0, 1.f);
		}
	} while (++NumSweepBounces < MAX_SWEEP_BOUNCES && !InOutVelocity.IsNearlyZero(0.1f));

	InOutTransform.SetLocation(CurrentLocation);
}


UCLASS()
class MASSTEST_API UCharacterToMassTranslatorProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	explicit UCharacterToMassTranslatorProcessor();

protected:
	//~ Begin UMassProcessor interface
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	//~ End UMassProcessor interface

private:
	FMassEntityQuery CharacterQuery;
};

inline UCharacterToMassTranslatorProcessor::UCharacterToMassTranslatorProcessor()
{
	bRequiresGameThreadExecution = false;
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::SyncWorldToMass;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
}

inline void UCharacterToMassTranslatorProcessor::ConfigureQueries()
{
	CharacterQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	CharacterQuery.AddRequirement<FActorHandleFragment>(EMassFragmentAccess::ReadOnly);
	CharacterQuery.AddTagRequirement<FCharacterMovementTag>(EMassFragmentPresence::All);
	CharacterQuery.RegisterWithProcessor(*this);
}

inline void UCharacterToMassTranslatorProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UCharacterToMassTranslatorProcessor::Execute"), STAT_CharacterToMassTranslator, STATGROUP_MassTest);

	CharacterQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context) -> void
	{
		const TArrayView<FTransformFragment> Transforms = Context.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FActorHandleFragment> Characters = Context.GetFragmentView<FActorHandleFragment>();

		for (int32 i = 0; i < Context.GetNumEntities(); ++i)
		{
			Transforms[i].GetMutableTransform() = Characters[i].Actor->GetActorTransform();
		}
	});
}


UCLASS()
class MASSTEST_API UMassToCharacterTranslatorProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	explicit UMassToCharacterTranslatorProcessor();

protected:
	//~ Begin UMassProcessor interface
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	//~ End UMassProcessor interface

private:
	FMassEntityQuery CharacterQuery;
};

inline UMassToCharacterTranslatorProcessor::UMassToCharacterTranslatorProcessor()
{
	bRequiresGameThreadExecution = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
}

inline void UMassToCharacterTranslatorProcessor::ConfigureQueries()
{
	CharacterQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	CharacterQuery.AddRequirement<FActorHandleFragment>(EMassFragmentAccess::ReadOnly);
	CharacterQuery.AddTagRequirement<FCharacterMovementTag>(EMassFragmentPresence::All);
	CharacterQuery.RegisterWithProcessor(*this);
}

inline void UMassToCharacterTranslatorProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UMassToCharacterTranslatorProcessor::Execute"), STAT_MassToCharacterTranslator, STATGROUP_MassTest);
	
	CharacterQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context) -> void
	{
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FActorHandleFragment> ActorHandles = Context.GetFragmentView<FActorHandleFragment>();

		for (int32 i = 0; i < Context.GetNumEntities(); ++i)
		{
			const FTransform& Transform = Transforms[i].GetTransform();
			AActor* Actor = ActorHandles[i].Actor;
			
			if (!ensure(IsValid(ActorHandles[i].Actor))) continue;

			Actor->SetActorLocationAndRotation(Transform.GetLocation(), Transform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		}
	});
}
