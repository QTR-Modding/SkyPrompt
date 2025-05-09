ScriptName SkyPromptTestScript Extends Quest

Int property clientID Auto

Event OnInit()
    clientID = SkyPrompt.RequestClientID()
    SkyPrompt.RegisterForSkyPrompt(clientID)

    Form reference = Game.GetPlayer() ; attach to player for testing

    Int eventID = 1
    Int actionID = 0
    Int type = 0 ; kSinglePress


    int[] devices = new int[2]
    devices[0] = 0
    devices[1] = 2
    int[] keys = new int[2]
    keys[0] = 0x23
    keys[1] = 0x8000
    
    SkyPrompt.SendPrompt(clientID, "Test Prompt", eventID, actionID, type, reference, devices, keys)
    
EndEvent
