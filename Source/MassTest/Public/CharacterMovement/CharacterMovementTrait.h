
#pragma once

#include "EntityCommon.h"
#include "MassCommonFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityTraitBase.h"
#include "CharacterMovementTrait.generated.h"

UCLASS()
class MASSTEST_API UCharacterMovementTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditAnywhere)
	float CapsuleHalfHeight = 88.f;

	UPROPERTY(EditAnywhere)
	float CapsuleRadius = 34.f;
	
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};

inline void UCharacterMovementTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FTransformFragment>();
	BuildContext.AddFragment<FVelocityFragment>();
	BuildContext.AddFragment<FMovementInputFragment>();
	BuildContext.AddFragment<FActorHandleFragment>();
	BuildContext.AddTag<FCharacterMovementTag>();
	BuildContext.AddTag<FGravityTag>();
	BuildContext.AddTag<FGroundedMovementTag>();

	FCapsuleFragment& CapsuleFragment = BuildContext.AddFragment_GetRef<FCapsuleFragment>();
	CapsuleFragment.HalfHeight = CapsuleHalfHeight;
	CapsuleFragment.Radius = CapsuleRadius;
}
