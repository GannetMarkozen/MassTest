#pragma once

#include "EntityCommon.h"
#include "MassProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "TestProcessor.generated.h"

UCLASS()
class MASSTEST_API UTestProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	explicit UTestProcessor();
	
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery Query;
};

inline UTestProcessor::UTestProcessor()
{
	bRequiresGameThreadExecution = false;
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}


inline void UTestProcessor::ConfigureQueries()
{
	Query.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	Query.AddTagRequirement<FCharacterMovementTag>(EMassFragmentPresence::None);

	Query.RegisterWithProcessor(*this);

}

inline void UTestProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("TestProcessor"), STAT_TestProcessor, STATGROUP_MassTest);

#if 0
	Query.ForEachEntityChunk(EntityManager, Context, [](FMassExecutionContext& Context) -> void
	{
		const TArrayView<FTransformFragment> Transforms = Context.GetMutableFragmentView<FTransformFragment>();
		for (int32 i = 0; i < Transforms.Num(); ++i)
		{
			FTransform& Transform = Transforms[i].GetMutableTransform();

			Transform.AddToTranslation(FVector::ForwardVector * 100.0 * Context.GetDeltaTimeSeconds());
		}
	});
#endif

#if 0
	UE::Mass::Utils::ForEach(Query, EntityManager, Context, [&Context](FTransformFragment& Transform) -> void
	{
		Transform.GetMutableTransform().AddToTranslation(FVector::ForwardVector * 100.0 * Context.GetDeltaTimeSeconds());
	});
#endif
}

