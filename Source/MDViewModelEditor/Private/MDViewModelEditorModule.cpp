#include "MDViewModelEditorModule.h"

#include "BlueprintEditorModule.h"
#include "BlueprintEditorTabs.h"
#include "PropertyEditorModule.h"
#include "UMGEditorModule.h"
#include "BlueprintModes/WidgetBlueprintApplicationMode.h"
#include "BlueprintModes/WidgetBlueprintApplicationModes.h"
#include "Components/MDViewModelAssignmentComponent.h"
#include "Customizations/MDViewModelAssignmentComponentCustomization.h"
#include "Customizations/MDViewModelAssignmentReferenceCustomization.h"
#include "EdGraphUtilities.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/MDViewModelSupportedInterface.h"
#include "ISettingsModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MDViewModelEditorConfig.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Widgets/SMDVMConfigEditor.h"
#include "ViewModelTab/MDViewModelTab.h"

#define LOCTEXT_NAMESPACE "FMDViewModelEditorModule"

class FMDViewModelGraphPanelPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* InPin) const override
	{
		if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && InPin->PinType.PinSubCategoryObject == TBaseStructure<FMDViewModelAssignmentReference>::Get())
		{
			return SNew(SMDViewModelAssignmentReferenceGraphPin, InPin);
		}

		return nullptr;
	}
};

void FMDViewModelEditorModule::StartupModule()
{
	IUMGEditorModule& UMGEditorModule = FModuleManager::LoadModuleChecked<IUMGEditorModule>("UMGEditor");
	UMGEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FMDViewModelEditorModule::HandleRegisterBlueprintEditorTab);

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FMDViewModelEditorModule::RegisterBlueprintEditorTab);
	BlueprintEditorModule.OnRegisterLayoutExtensions().AddRaw(this, &FMDViewModelEditorModule::RegisterBlueprintEditorLayout);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(FMDViewModelAssignmentReference::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMDViewModelAssignmentReferenceCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomClassLayout(UMDViewModelAssignmentComponent::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FMDViewModelAssignmentComponentCustomization::MakeInstance));

	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
	if (FSlateApplication::IsInitialized())
	{
		SettingsModule.RegisterSettings(TEXT("Project"), TEXT("Game"), TEXT("ViewModelConfig"), INVTEXT("View Model Config Properties"), INVTEXT("Set the values of Config properties on view model classes"), SAssignNew(ViewModelConfigEditor, SMDVMConfigEditor));

		FCoreDelegates::OnPreExit.AddLambda([]()
		{
			// Must unregister before other modules shutdown so dependent widgets (eg. gameplay tag customization) can clean up properly
			if (FMDViewModelEditorModule* ThisModule = FModuleManager::GetModulePtr<FMDViewModelEditorModule>("MDViewModelEditor"))
			{
				ThisModule->UnregisterConfigEditor();
			}
		});
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetEditorOpened().AddRaw(this, &FMDViewModelEditorModule::RegisterBlueprintEditorDrawer);

	ViewModelGraphPanelPinFactory = MakeShared<FMDViewModelGraphPanelPinFactory>();
	FEdGraphUtilities::RegisterVisualPinFactory(ViewModelGraphPanelPinFactory);
}

void FMDViewModelEditorModule::ShutdownModule()
{
	if (FBlueprintEditorModule* BlueprintEditorModule = FModuleManager::GetModulePtr<FBlueprintEditorModule>("Kismet"))
	{
		BlueprintEditorModule->OnRegisterTabsForEditor().RemoveAll(this);
		BlueprintEditorModule->OnRegisterLayoutExtensions().RemoveAll(this);
	}

	if (GEditor)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OnAssetEditorOpened().RemoveAll(this);
		}
	}

	if (ViewModelGraphPanelPinFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(ViewModelGraphPanelPinFactory);
	}

	if (IUMGEditorModule* UMGEditorModule = FModuleManager::GetModulePtr<IUMGEditorModule>("UMGEditor"))
	{
		UMGEditorModule->OnRegisterTabsForEditor().RemoveAll(this);
	}

	if (FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		PropertyEditorModule->UnregisterCustomPropertyTypeLayout(FMDViewModelAssignmentReference::StaticStruct()->GetFName());
		PropertyEditorModule->UnregisterCustomPropertyTypeLayout(UMDViewModelAssignmentComponent::StaticClass()->GetFName());
	}
}

void FMDViewModelEditorModule::UnregisterConfigEditor()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings(TEXT("Project"), TEXT("Game"), TEXT("ViewModelConfig"));
		ViewModelConfigEditor.Reset();
	}
}

void FMDViewModelEditorModule::HandleRegisterBlueprintEditorTab(const FWidgetBlueprintApplicationMode& InApplicationMode, FWorkflowAllowedTabSet& TabFactories)
{
	// Don't allow View Model Editor in Debug/Preview mode
	if (InApplicationMode.GetModeName() == FWidgetBlueprintApplicationModes::DesignerMode
		|| InApplicationMode.GetModeName() == FWidgetBlueprintApplicationModes::GraphMode)
	{
		constexpr bool bIsDrawer = false;
		TabFactories.RegisterFactory(MakeShared<FMDViewModelSummoner>(InApplicationMode.GetBlueprintEditor(), bIsDrawer));

		if (InApplicationMode.LayoutExtender)
		{
			const FName RelativeTab = InApplicationMode.GetModeName() == FWidgetBlueprintApplicationModes::DesignerMode
				? TEXT("Animations")
				: FBlueprintEditorTabs::FindResultsID;
			const FTabManager::FTab NewTab(FTabId(FMDViewModelSummoner::TabID, ETabIdFlags::SaveLayout), ETabState::ClosedTab);
			InApplicationMode.LayoutExtender->ExtendLayout(RelativeTab, ELayoutExtensionPosition::After, NewTab);

			InApplicationMode.OnPostActivateMode.AddRaw(this, &FMDViewModelEditorModule::HandleActivateMode);
			InApplicationMode.OnPreDeactivateMode.AddRaw(this, &FMDViewModelEditorModule::HandleDeactivateMode);
		}
	}
}

void FMDViewModelEditorModule::HandleActivateMode(FWidgetBlueprintApplicationMode& InApplicationMode)
{
	if (const TSharedPtr<FWidgetBlueprintEditor> BlueprintEditor = InApplicationMode.GetBlueprintEditor())
	{
		// Don't allow View Model Editor in Debug/Preview mode
		if (InApplicationMode.GetModeName() == FWidgetBlueprintApplicationModes::DesignerMode
			|| InApplicationMode.GetModeName() == FWidgetBlueprintApplicationModes::GraphMode)
		{
			BlueprintEditor->RegisterDrawer(FMDViewModelSummoner::CreateDrawerConfig(BlueprintEditor.ToSharedRef()), INDEX_NONE);
		}
	}
}

void FMDViewModelEditorModule::HandleDeactivateMode(FWidgetBlueprintApplicationMode& InApplicationMode)
{
	TSharedPtr<FWidgetBlueprintEditor> BP = InApplicationMode.GetBlueprintEditor();
	if (BP && BP->IsEditorClosing())
	{
		InApplicationMode.OnPostActivateMode.RemoveAll(this);
		InApplicationMode.OnPreDeactivateMode.RemoveAll(this);
	}
}

void FMDViewModelEditorModule::RegisterBlueprintEditorLayout(FLayoutExtender& Extender)
{
	if (!GetDefault<UMDViewModelEditorConfig>()->bEnableViewModelsInActorBlueprints)
	{
		return;
	}

	Extender.ExtendLayout(FBlueprintEditorTabs::FindResultsID, ELayoutExtensionPosition::Before, FTabManager::FTab(FMDViewModelSummoner::TabID, ETabState::ClosedTab));
}

void FMDViewModelEditorModule::RegisterBlueprintEditorTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	if (InModeName != FBlueprintEditorApplicationModes::StandardBlueprintEditorMode)
	{
		return;
	}

	if (!GetDefault<UMDViewModelEditorConfig>()->bEnableViewModelsInActorBlueprints || !BlueprintEditor.IsValid())
	{
		return;
	}

	const UBlueprint* Blueprint = BlueprintEditor->GetBlueprintObj();
	if (!IsValid(Blueprint))
	{
		return;
	}

	if (!IsValid(Blueprint->GeneratedClass) || (!Blueprint->GeneratedClass->IsChildOf<AActor>() && !Blueprint->GeneratedClass->ImplementsInterface(UMDViewModelSupportedInterface::StaticClass())))
	{
		return;
	}

	constexpr bool bIsDrawer = false;
	TabFactories.RegisterFactory(MakeShared<FMDViewModelSummoner>(BlueprintEditor, bIsDrawer));
}

void FMDViewModelEditorModule::RegisterBlueprintEditorDrawer(UObject* Asset)
{
	if (!GetDefault<UMDViewModelEditorConfig>()->bEnableViewModelsInActorBlueprints)
	{
		return;
	}

	const UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!IsValid(AssetEditorSubsystem) || !IsValid(Blueprint))
	{
		return;
	}

	if (!IsValid(Blueprint->GeneratedClass) || (!Blueprint->GeneratedClass->IsChildOf<AActor>() && !Blueprint->GeneratedClass->ImplementsInterface(UMDViewModelSupportedInterface::StaticClass())))
	{
		return;
	}

	static const FName BlueprintEditorName = TEXT("BlueprintEditor");
	constexpr bool bFocus = false;
	IAssetEditorInstance* Editor = AssetEditorSubsystem->FindEditorForAsset(Asset, bFocus);
	if (Editor == nullptr || Editor->GetEditorName() != BlueprintEditorName)
	{
		return;
	}

	const TSharedRef<FBlueprintEditor> BlueprintEditor = StaticCastSharedRef<FBlueprintEditor>(static_cast<FBlueprintEditor*>(Editor)->AsShared());
	if (!BlueprintEditor->IsModeCurrent(FBlueprintEditorApplicationModes::StandardBlueprintEditorMode))
	{
		return;
	}

	BlueprintEditor->RegisterDrawer(FMDViewModelSummoner::CreateDrawerConfig(BlueprintEditor));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMDViewModelEditorModule, MDViewModelEditor)
