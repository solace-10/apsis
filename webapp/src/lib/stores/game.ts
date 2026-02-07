/**
 * Svelte stores for Orbis game state.
 * Provides reactive access to game data from WASM module.
 */

import { writable, derived, type Readable } from 'svelte/store';
import type { SpaceObject, SpaceObjectGroup, OverlayState, OrbisModule } from '$lib/wasm/types';
import {
	getModule,
	onStateChange,
	loadModule,
	registerCallbacks,
	initializeCallbacks,
	callSetGroupFilterEnabled
} from '$lib/wasm/module';

// Module instance store
const moduleStore = writable<OrbisModule | null>(null);

// Selected object store
const selectedObjectStore = writable<SpaceObject | null>(null);

// Groups store
const groupsStore = writable<SpaceObjectGroup[]>([]);

// Overlay state store
const overlayStore = writable<OverlayState>({
	grid: false,
	atmosphere: true,
	labels: true,
	orbits: true,
	groundTracks: false
});

// Module loading state
export const moduleLoading = writable<boolean>(true);
export const moduleError = writable<string | null>(null);

/**
 * Initialize stores from module state.
 */
function syncFromModule(module: OrbisModule): void {
	selectedObjectStore.set(module.getSelectedObject());
	groupsStore.set(module.getGroups());
	overlayStore.set(module.getOverlayState());
}

/**
 * Update groups store from C++ interop data.
 */
export function setGroupsFromInterop(groups: SpaceObjectGroup[]): void {
	groupsStore.set(groups);
}

/**
 * Set up C++ -> JS callbacks for space object updates.
 * Must be called before WASM module loads.
 */
export function setupWasmCallbacks(): void {
	registerCallbacks(
		(object: SpaceObject | null) => {
			selectedObjectStore.set(object);
		},
		(object: SpaceObject | null) => {
			selectedObjectStore.set(object);
		},
		(groups: SpaceObjectGroup[]) => {
			setGroupsFromInterop(groups);
		}
	);
	initializeCallbacks();
}

/**
 * Initialize the game stores by loading the WASM module.
 */
export async function initializeGame(): Promise<void> {
	moduleLoading.set(true);
	moduleError.set(null);

	try {
		const module = await loadModule();
		moduleStore.set(module);
		syncFromModule(module);

		// Subscribe to state changes from module
		onStateChange(() => {
			syncFromModule(module);
		});

		moduleLoading.set(false);
	} catch (err) {
		moduleError.set(err instanceof Error ? err.message : 'Failed to load module');
		moduleLoading.set(false);
	}
}

/**
 * Selected space object (readonly).
 */
export const selectedObject: Readable<SpaceObject | null> = {
	subscribe: selectedObjectStore.subscribe
};

/**
 * All object groups (readonly).
 */
export const groups: Readable<SpaceObjectGroup[]> = {
	subscribe: groupsStore.subscribe
};

/**
 * Visible groups only.
 */
export const visibleGroups: Readable<SpaceObjectGroup[]> = derived(groups, ($groups) =>
	$groups.filter((g) => g.visible)
);

/**
 * Overlay state (readonly).
 */
export const overlays: Readable<OverlayState> = {
	subscribe: overlayStore.subscribe
};

/**
 * Select a space object by ID.
 */
export function selectObject(id: number | null): void {
	const module = getModule();
	module.selectObject(id);
}

/**
 * Toggle visibility of an object group.
 */
export function toggleGroupVisibility(groupId: string): void {
	let currentVisible = false;
	groups.subscribe((g) => {
		const group = g.find((gr) => gr.id === groupId);
		if (group) {
			currentVisible = group.visible;
		}
	})();
	callSetGroupFilterEnabled(groupId, !currentVisible);
}

/**
 * Set visibility of an object group.
 */
export function setGroupVisibility(groupId: string, visible: boolean): void {
	callSetGroupFilterEnabled(groupId, visible);
}

/**
 * Toggle an overlay.
 */
export function toggleOverlay(overlay: keyof OverlayState): void {
	const module = getModule();
	const currentState = module.getOverlayState();
	module.setOverlay(overlay, !currentState[overlay]);
}

/**
 * Set overlay state.
 */
export function setOverlay(overlay: keyof OverlayState, enabled: boolean): void {
	const module = getModule();
	module.setOverlay(overlay, enabled);
}

/**
 * Get all space objects (not reactive, call as needed).
 */
export function getSpaceObjects(): SpaceObject[] {
	const module = getModule();
	return module.getSpaceObjects();
}

/**
 * Search space objects by name.
 */
export function searchObjects(query: string): SpaceObject[] {
	if (!query.trim()) return [];
	const lowerQuery = query.toLowerCase();
	return getSpaceObjects().filter(
		(obj) =>
			obj.name.toLowerCase().includes(lowerQuery) ||
			obj.noradId.toString().includes(query)
	);
}
