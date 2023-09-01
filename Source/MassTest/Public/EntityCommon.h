
#pragma once

#include "MassDebugger.h"
#include "MassEntityTypes.h"
#include "MassExecutionContext.h"
#include "EntityCommon.generated.h"

DECLARE_STATS_GROUP(TEXT("MassTest"), STATGROUP_MassTest, STATCAT_Advanced);

USTRUCT()
struct MASSTEST_API FCapsuleFragment : public FMassFragment
{
	GENERATED_BODY()

	float HalfHeight = 88.f;
	float Radius = 34.f;
};

USTRUCT()
struct MASSTEST_API FVelocityFragment : public FMassFragment
{
	GENERATED_BODY()

	FVector3f Velocity = FVector3f::ZeroVector;
};

USTRUCT()
struct MASSTEST_API FMovementInputFragment : public FMassFragment
{
	GENERATED_BODY()

	FORCEINLINE FMovementInputFragment& operator=(const FVector2f& Other) { MovementInput = Other; return *this; }

	FVector2f MovementInput = FVector2f::ZeroVector;
};

USTRUCT()
struct MASSTEST_API FActorHandleFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* Actor = nullptr;
};

USTRUCT()
struct MASSTEST_API FCharacterMovementTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct MASSTEST_API FGroundedMovementTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct MASSTEST_API FGravityTag : public FMassTag
{
	GENERATED_BODY()
};

namespace UE::Mass::ProcessorGroupNames
{
	inline const FName ProcessInput{TEXT("ProcessInput")};
}

template <typename T1, typename T2>
FORCEINLINE auto operator-=(UE::Math::TVector<T1>& A, const UE::Math::TVector<T2>& B) -> UE::Math::TVector<T1>&
{
	A.X -= B.X;
	A.Y -= B.Y;
	A.Z -= B.Z;
	return A;
}

template <typename T1, typename T2>
FORCEINLINE auto operator+(const UE::Math::TVector<T1>& A, const UE::Math::TVector<T2>& B) -> UE::Math::TVector<decltype(DeclVal<T1>() + DeclVal<T2>())>
{
	return UE::Math::TVector<decltype(DeclVal<T1>() + DeclVal<T2>())>{A.X + B.X, A.Y + B.Y, A.Z + B.Z};
}

template <typename T1, typename T2>
FORCEINLINE auto operator*(const UE::Math::TVector<T1>& A, const UE::Math::TVector<T2>& B) -> UE::Math::TVector<decltype(DeclVal<T1>() * DeclVal<T2>())>
{
	return UE::Math::TVector<decltype(DeclVal<T1>() * DeclVal<T2>())>{A.X * B.X, A.Y * B.Y, A.Z * B.Z};
}

template <typename T1, typename T2>
FORCEINLINE auto operator|(const UE::Math::TVector<T1>& A, const UE::Math::TVector<T2>& B) -> decltype(DeclVal<T1>() * DeclVal<T2>())
{
	return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}