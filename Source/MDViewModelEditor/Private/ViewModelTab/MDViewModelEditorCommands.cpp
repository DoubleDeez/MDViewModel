#include "ViewModelTab/MDViewModelEditorCommands.h"

void FMDViewModelEditorCommands::RegisterCommands()
{
	FUICommandInfo::MakeCommandInfo(
		AsShared(),
		Edit,
		TEXT("Edit"),
		INVTEXT("Edit Assignment"),
		INVTEXT("Opens the view model assignment dialog to edit the selected assignment."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Edit")),
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	FUICommandInfo::MakeCommandInfo(
		AsShared(),
		GoToDefinition,
		TEXT("GoToDefinition"),
		INVTEXT("Go To Definition"),
		INVTEXT("Open the C++ file or Blueprint where this item is defined."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.ArrowRight")),
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Alt, EKeys::G)
	);
}
