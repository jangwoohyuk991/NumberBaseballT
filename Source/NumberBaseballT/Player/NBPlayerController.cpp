#include "Player/NBPlayerController.h"
#include "UI/NBChatInput.h"
#include "Kismet/KismetSystemLibrary.h"

void ANBPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    if (!IsValid(ChatInputWidgetClass))
    {
        return;
    }

    ChatInputWidgetInstance =
        CreateWidget<UNBChatInput>(this, ChatInputWidgetClass);

    if (!IsValid(ChatInputWidgetInstance))
    {
        return;
    }

    ChatInputWidgetInstance->AddToViewport();

    bShowMouseCursor = true;

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(ChatInputWidgetInstance->TakeWidget());
    SetInputMode(InputMode);
}

void ANBPlayerController::SetChatMessageString(
    const FString& InChatMessageString)
{
    ChatMessageString = InChatMessageString;
    PrintChatMessageString(ChatMessageString);
}

void ANBPlayerController::PrintChatMessageString(
    const FString& InChatMessageString)
{
    UKismetSystemLibrary::PrintString(
        this,
        InChatMessageString,
        true,
        true,
        FLinearColor::Red,
        5.0f);
}
