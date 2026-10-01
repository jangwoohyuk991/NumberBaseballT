#include "Player/NBPawn.h"

#include "NumberBaseballT.h"

void ANBPawn::BeginPlay()
{
    Super::BeginPlay();

    FString NetRoleString =
        NumberBaseballTFunctionLibrary::GetRoleString(this);

    FString CombinedString = FString::Printf(
        TEXT("NBPawn::BeginPlay() %s [%s]"),
        *NumberBaseballTFunctionLibrary::GetNetModeString(this),
        *NetRoleString);

    NumberBaseballTFunctionLibrary::MyPrintString(
        this,
        CombinedString,
        10.f);
}

void ANBPawn::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    FString NetRoleString =
        NumberBaseballTFunctionLibrary::GetRoleString(this);

    FString CombinedString = FString::Printf(
        TEXT("NBPawn::PossessedBy() %s [%s]"),
        *NumberBaseballTFunctionLibrary::GetNetModeString(this),
        *NetRoleString);

    NumberBaseballTFunctionLibrary::MyPrintString(
        this,
        CombinedString,
        10.f);
}