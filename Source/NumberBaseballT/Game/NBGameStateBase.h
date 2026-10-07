#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NBGameStateBase.generated.h"

UCLASS()
class NUMBERBASEBALLT_API ANBGameStateBase : public AGameStateBase
{
    GENERATED_BODY()

public:
    ANBGameStateBase();

    virtual void GetLifetimeReplicatedProps(
        TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(NetMulticast, Reliable)
    void MulticastRPCBroadcastLoginMessage(
        const FString& InNameString = FString(TEXT("XXXXXXX")));

public:
    // 서버에서 관리하고 모든 클라이언트에 전달하는 남은 시간.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
    int32 RemainingTurnTime;

    // 서버에서 지정하고 모든 클라이언트에 전달하는 현재 턴 이름.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
    FString CurrentTurnPlayerName;
};