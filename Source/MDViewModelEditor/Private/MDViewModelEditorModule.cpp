#include "MDViewModelEditorModule.h"

#include "BlueprintCompilationManager.h"
#include "BlueprintEditorTabs.h"
#include "PropertyEditorModule.h"
#include "UMGEditorModule.h"
#include "WidgetBlueprint.h"
#include "WidgetDrawerConfig.h"
#include "BlueprintModes/WidgetBlueprintApplicationMode.h"
#include "BlueprintModes/WidgetBlueprintApplicationModes.h"
#include "Customizations/MDViewModelAssignmentReferenceCustomization.h"
#include "Framework/Application/SlateApplication.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "ViewModelTab/MDViewModelTab.h"
#include "WidgetExtensions/MDViewModelBlueprintCompilerExtension.h"

#define LOCTEXT_NAMESPACE "FMDViewModelEditorModule"

void FMDViewModelEditorModule::StartupModule()
{
	IUMGEditorModule& UMGEditorModule = FModuleManager::LoadModuleChecked<IUMGEditorModule>("UMGEditor");
	UMGEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FMDViewModelEditorModule::HandleRegisterBlueprintEditorTab);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(FMDViewModelAssignmentReference::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMDViewModelAssignmentReferenceCustomization::MakeInstance));

	CompilerExtension = NewObject<UMDViewModelBlueprintCompilerExtension>();
	CompilerExtension->AddToRoot();

	FBlueprintCompilationManager::RegisterCompilerExtension(UWidgetBlueprint::StaticClass(), CompilerExtension);
}

void FMDViewModelEditorModule::ShutdownModule()
{
	if (IsValid(CompilerExtension))
	{
		CompilerExtension->RemoveFromRoot();
		CompilerExtension = nullptr;
	}

	if (IUMGEditorModule* UMGEditorModule = FModuleManager::GetModulePtr<IUMGEditorModule>("UMGEditor"))
	{
		UMGEditorModule->OnRegisterTabsForEditor().RemoveAll(this);
	}

	if (FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		PropertyEditorModule->UnregisterCustomPropertyTypeLayout(FMDViewModelAssignmentReference::StaticStruct()->GetFName());
	}
}

void FMDViewModelEditorModule::HandleRegisterBlueprintEditorTab(const FWidgetBlueprintApplicationMode& ApplicationMode, FWorkflowAllowedTabSet& TabFactories)
{
	if (ApplicationMode.LayoutExtender)
	{
		TabFactories.RegisterFactory(MakeShared<FMDViewModelSummoner>(ApplicationMode.GetBlueprintEditor()));

		const FName RelativeTab = ApplicationMode.GetModeName() == FWidgetBlueprintApplicationModes::DesignerMode
			? TEXT("Animations")
			: FBlueprintEditorTabs::FindResultsID;
		const FTabManager::FTab NewTab(FTabId(FMDViewModelSummoner::TabID, ETabIdFlags::SaveLayout), ETabState::ClosedTab);
		ApplicationMode.LayoutExtender->ExtendLayout(RelativeTab, ELayoutExtensionPosition::After, NewTab);

		ApplicationMode.OnPostActivateMode.AddRaw(this, &FMDViewModelEditorModule::HandleActivateMode);
		ApplicationMode.OnPreDeactivateMode.AddRaw(this, &FMDViewModelEditorModule::HandleDeactivateMode);
	}
}

void FMDViewModelEditorModule::HandleActivateMode(FWidgetBlueprintApplicationMode& InDesignerMode)
{
	if (TSharedPtr<FWidgetBlueprintEditor> BP = InDesignerMode.GetBlueprintEditor())
	{
		if (!BP->GetExternalEditorWidget(FMDViewModelSummoner::DrawerID))
		{
			const FMDViewModelSummoner MDVMDrawerSummoner(BP);
			const FWorkflowTabSpawnInfo SpawnInfo;
			BP->AddExternalEditorWidget(FMDViewModelSummoner::DrawerID, MDVMDrawerSummoner.CreateTabBody(SpawnInfo));
		}

		FWidgetDrawerConfig MDVMDrawer(FMDViewModelSummoner::DrawerID);
		TWeakPtr<FWidgetBlueprintEditor> WeakBP = BP;
		MDVMDrawer.GetDrawerContentDelegate.BindLambda([WeakBP]()
		{
			if (TSharedPtr<FWidgetBlueprintEditor> BP = WeakBP.Pin())
			{
				if (const TSharedPtr<SWidget> DrawerWidgetContent = BP->GetExternalEditorWidget(FMDViewModelSummoner::DrawerID))
				{
					return DrawerWidgetContent.ToSharedRef();
				}
			}

			return SNullWidget::NullWidget;
		});
		MDVMDrawer.OnDrawerOpenedDelegate.BindLambda([WeakBP](FName StatusBarWithDrawerName)
		{
			if (TSharedPtr<FWidgetBlueprintEditor> BP = WeakBP.Pin())
			{
				FSlateApplication::Get().SetUserFocus(FSlateApplication::Get().GetUserIndexForKeyboard(), BP->GetExternalEditorWidget(FMDViewModelSummoner::DrawerID));
			}
		});
		MDVMDrawer.OnDrawerDismissedDelegate.BindLambda([WeakBP](const TSharedPtr<SWidget>& NewlyFocusedWidget)
		{
			if (TSharedPtr<FWidgetBlueprintEditor> BP = WeakBP.Pin())
			{
				BP->SetKeyboardFocus();
			}
		});
		MDVMDrawer.ButtonText = LOCTEXT("ViewsModels", "View Models");
		MDVMDrawer.ToolTipText = LOCTEXT("ViewsModelsToolTip", "Modify which view models are assigned to this widget");
		MDVMDrawer.Icon = FAppStyle::GetBrush(TEXT("FontEditor.Tabs.PageProperties"));
		BP->RegisterDrawer(MoveTemp(MDVMDrawer), INDEX_NONE);
	}
}

void FMDViewModelEditorModule::HandleDeactivateMode(FWidgetBlueprintApplicationMode& InDesignerMode)
{
	TSharedPtr<FWidgetBlueprintEditor> BP = InDesignerMode.GetBlueprintEditor();
	if (BP && BP->IsEditorClosing())
	{
		InDesignerMode.OnPostActivateMode.RemoveAll(this);
		InDesignerMode.OnPreDeactivateMode.RemoveAll(this);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMDViewModelEditorModule, MDViewModelEditor)