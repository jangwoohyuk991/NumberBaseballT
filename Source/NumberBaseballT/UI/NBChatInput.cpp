#include "UI/NBChatInput.h"
#include "Components/EditableTextBox.h"
#include "Player/NBPlayerController.h"

void UNBChatInput::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(EditableTextBox_ChatInput))
    {
        EditableTextBox_ChatInput->OnTextCommitted.AddUniqueDynamic(
            this,
            &UNBChatInput::OnChatInputTextCommitted);
    }
}

void UNBChatInput::NativeDestruct()
{
    if (IsValid(EditableTextBox_ChatInput))
    {
        EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(
            this,
            &UNBChatInput::OnChatInputTextCommitted);
    }

    Super::NativeDestruct();
}

void UNBChatInput::OnChatInputTextCommitted(
    const FText& Text,
    ETextCommit::Type CommitMethod)
{
    if (CommitMethod != ETextCommit::OnEnter)
    {
        return;
    }

    ANBPlayerController* OwningController =
        Cast<ANBPlayerController>(GetOwningPlayer());

    if (!IsValid(OwningController))
    {
        return;
    }

    OwningController->SetChatMessageString(Text.ToString());

    EditableTextBox_ChatInput->SetText(FText::GetEmpty());
}