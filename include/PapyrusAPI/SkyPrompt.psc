Scriptname SkyPrompt

Int Function RequestClientID() global native
Function RegisterForSkyPrompt(Int clientID) global native
Function UnregisterFromSkyPrompt(Int clientID) global native
Bool Function SendPrompt(Int clientID, String text, Int eventID, Int actionID, Int type, Form refForm, Int[] devices, Int[] keys) global native
Function RemovePrompt(Int clientID, Int eventID, Int actionID) global native
