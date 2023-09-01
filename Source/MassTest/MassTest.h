// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#define PRINT(Fmt, ...) { if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, FString::Printf(TEXT(Fmt), ##__VA_ARGS__)); }