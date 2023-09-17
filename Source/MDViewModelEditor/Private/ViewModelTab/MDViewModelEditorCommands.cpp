#include "ViewModelTab/MDViewModelEditorCommands.h"

#define LOCTEXT_NAMESPACE "MDViewModelEditorCommands"

void FMDViewModelEditorCommands::RegisterCommands()
{
	UI_COMMAND(Edit, "Edit Assignment", "Opens the view model assignment dialog to edit the selected assignment.", EUserInterfaceActionType::Button, FInputChord())
}

#undef LOCTEXT_NAMESPACE
