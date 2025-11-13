# Upstream FreeIntv Update Strategy

## Current Situation

**Upstream Changes Found**: 1 commit
- **Commit**: `1b51f41` - "Add commit hash to the version string (#89)"
- **Date**: Nov 11, 2025
- **File Modified**: `src/libretro.c` (5 lines changed)

## What's Changing

The upstream update modifies the version string in `retro_get_system_info()` to include a git commit hash:

### Before (your current version):
```c
info->library_version = "1.2";
```

### After (upstream):
```c
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
info->library_version = "1.2 " GIT_VERSION;
```

This allows the build system to inject the commit hash at compile time.

## Merge Strategy Options

### Option 1: Git Rebase (Recommended - Clean History)
```powershell
git rebase upstream/master
```

**Pros**:
- Clean, linear history
- Your touchscreen commits appear on top of upstream
- Easy to understand progression

**Cons**:
- Rewrites history (only do if commits haven't been pushed)

### Option 2: Git Merge (Traditional)
```powershell
git merge upstream/master
```

**Pros**:
- Preserves complete history
- Non-destructive
- Standard workflow

**Cons**:
- Creates a merge commit
- Slightly messier history

### Option 3: Cherry-pick (If Conflicts Exist)
Only if there are conflicts that need manual resolution.

## Conflict Risk Assessment

**Risk Level**: 🟢 **LOW**

**Why**:
1. The change is in `retro_get_system_info()` (around line 442)
2. Your touchscreen code is in `render_dual_screen()`, `process_hotspot_input()`, `process_utility_button_input()`, and `retro_run()` main loop
3. No code overlap with the version string change
4. Very unlikely to have conflicts

## Recommended Approach

**Use Option 2 (Git Merge)** because:
- ✅ Non-destructive
- ✅ Preserves all history
- ✅ Low conflict risk
- ✅ Easy to understand the integration point
- ✅ Best for collaborative work

## Steps to Execute

1. **Check current status** (ensure everything is committed):
   ```powershell
   git status
   ```

2. **Perform the merge**:
   ```powershell
   git merge upstream/master
   ```

3. **If conflicts occur** (unlikely):
   ```powershell
   # Resolve conflicts in your editor
   # Then:
   git add src/libretro.c
   git commit -m "Merge upstream: Add commit hash to version string"
   ```

4. **If no conflicts** (most likely):
   ```powershell
   # Git will auto-merge and create a merge commit
   # Just verify the result
   git log --oneline -5
   ```

5. **Verify the merge**:
   ```powershell
   # Check the merged version string code
   git show HEAD:src/libretro.c | grep -A 10 "retro_get_system_info"
   
   # Verify your touchscreen code is still intact
   git show HEAD:src/libretro.c | grep -c "fullscreen_mode"
   # Should return a non-zero count
   ```

6. **Rebuild and test**:
   ```powershell
   make platform=unix
   # Test in RetroArch
   ```

## What to Watch For

### Your Touchscreen Code Preservation
Your modifications in `src/libretro.c` include:
- ✅ `FULLSCREEN_*` constants (lines ~30-35)
- ✅ `fullscreen_mode`, `fullscreen_strip_visible`, `fullscreen_hide_timer` globals
- ✅ `utility_buttons[7]` with button 3 at index 3
- ✅ `utility_button_images[7]` 
- ✅ `button_filenames[7]` with "button_full_screen_toggle.png"
- ✅ Modified `process_hotspot_input()` with ~70 lines of fullscreen logic
- ✅ Modified `render_dual_screen()` with ~85 lines of fullscreen rendering
- ✅ Modified `process_utility_button_input()` with fullscreen case
- ✅ Modified `retro_run()` with timer management

**All of these should be preserved during the merge** because the upstream change only touches the version string (lines ~442-448).

## Post-Merge Verification

Run this quick check:
```powershell
cd s:\VisualStudio\FreeIntvTSOverlay
git log --oneline -3
# Should show your commits + merge commit + upstream commit
```

## If Something Goes Wrong

If you need to abort the merge:
```powershell
git merge --abort
```

Then come back to this document and try Option 1 (rebase) instead.

---

**Ready to merge?** Run the commands in the "Steps to Execute" section above.
