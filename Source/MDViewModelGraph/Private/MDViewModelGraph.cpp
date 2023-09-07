#include "BlueprintCompilationManager.h"
#include "BlueprintExtensions/MDViewModelAssignableInterface.h"
#include "BlueprintExtensions/MDViewModelBlueprintCompilerExtension.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "Util/MDViewModelGraphStatics.h"


class FMDViewModelGraphModule : public IModuleInterface
{
	virtual void StartupModule() override
	{
		FCoreDelegates::OnPostEngineInit.AddLambda([this]()
		{
			if (GEditor != nullptr)
			{
				PreCompileHandle = GEditor->OnBlueprintPreCompile().AddRaw(this, &FMDViewModelGraphModule::OnBlueprintPreCompile);
			}
		});

		CompilerExtensionPtr = NewObject<UMDViewModelBlueprintCompilerExtension>();
		CompilerExtensionPtr->AddToRoot();

		FBlueprintCompilationManager::RegisterCompilerExtension(UBlueprint::StaticClass(), CompilerExtensionPtr.Get());
	}
	
	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnPostEngineInit.RemoveAll(this);
		if (GEditor != nullptr)
		{
			GEditor->OnBlueprintPreCompile().Remove(PreCompileHandle);
			PreCompileHandle.Reset();
		}
	
		if (UMDViewModelBlueprintCompilerExtension* CompilerExtension = CompilerExtensionPtr.Get())
		{
			CompilerExtension->RemoveFromRoot();
			CompilerExtension = nullptr;
		}
	}

	void OnBlueprintPreCompile(UBlueprint* Blueprint)
	{
		if (IsValid(Blueprint) && IsValid(Blueprint->ParentClass) && Blueprint->ParentClass->IsChildOf<AActor>())
		{
			if (UMDViewModelBlueprintCompilerExtension* CompilerExtension = CompilerExtensionPtr.Get())
			{
				IMDViewModelAssignableInterface* Extension = FMDViewModelGraphStatics::GetAssignableInterface(Blueprint);
				if (Extension == nullptr || Extension->GetAssignments().IsEmpty())
				{
					return;
				}

				CompilerExtension->HandleActorBlueprintPreCompile(Extension, Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass));
			}
		}
	}

	FDelegateHandle PreCompileHandle;

	// Use a weak ptr even though we add it to root, it can be destroyed before the module shuts down
	TWeakObjectPtr<UMDViewModelBlueprintCompilerExtension> CompilerExtensionPtr = nullptr;
};

IMPLEMENT_MODULE(FMDViewModelGraphModule, MDViewModelGraph)