/**
 * WASM module wrapper with mock data for development.
 * Replace mock implementations with real embind calls when C++ bindings are ready.
 */

import type {
	SpaceObject,
	SpaceObjectGroup,
	OverlayState,
	OrbisModule,
	SpaceObjectInterop
} from './types';
import { fromInterop } from './types';

/**
 * Callback type for space object selection.
 */
export type SpaceObjectCallback = (object: SpaceObject | null) => void;
export type GroupFiltersCallback = (groups: SpaceObjectGroup[]) => void;

// Callbacks registered by the app
let onSelectionChange: SpaceObjectCallback | null = null;
let onObjectUpdate: SpaceObjectCallback | null = null;
let onGroupFiltersChange: GroupFiltersCallback | null = null;

/**
 * Register callbacks for space object updates from C++.
 */
export function registerCallbacks(
	onSelected: SpaceObjectCallback,
	onUpdated: SpaceObjectCallback,
	onGroupFilters?: GroupFiltersCallback
): void {
	onSelectionChange = onSelected;
	onObjectUpdate = onUpdated;
	onGroupFiltersChange = onGroupFilters ?? null;
}

/**
 * Initialize the global orbisCallbacks object that C++ will call into.
 * Must be called before WASM module loads.
 */
export function initializeCallbacks(): void {
	interface OrbisCallbacks {
		onSpaceObjectSelected: (interop: SpaceObjectInterop) => void;
		onSpaceObjectDeselected: () => void;
		onSpaceObjectUpdated: (interop: SpaceObjectInterop) => void;
		onGroupFiltersChanged: (groups: SpaceObjectGroup[]) => void;
	}

	const callbacks: OrbisCallbacks = {
		onSpaceObjectSelected: (interop: SpaceObjectInterop) => {
			const spaceObject = fromInterop(interop);
			if (onSelectionChange) {
				onSelectionChange(spaceObject);
			}
		},
		onSpaceObjectDeselected: () => {
			if (onSelectionChange) {
				onSelectionChange(null);
			}
		},
		onSpaceObjectUpdated: (interop: SpaceObjectInterop) => {
			const spaceObject = fromInterop(interop);
			if (onObjectUpdate) {
				onObjectUpdate(spaceObject);
			}
		},
		onGroupFiltersChanged: (groups: SpaceObjectGroup[]) => {
			if (onGroupFiltersChange) {
				onGroupFiltersChange(groups);
			}
		}
	};

	(window as unknown as { orbisCallbacks: OrbisCallbacks }).orbisCallbacks = callbacks;
}

let overlayState: OverlayState = {
	grid: false,
	atmosphere: true,
	labels: true,
	orbits: true,
	groundTracks: false
};

// State change callbacks
type StateChangeCallback = () => void;
const stateChangeCallbacks: StateChangeCallback[] = [];

function notifyStateChange(): void {
	for (const callback of stateChangeCallbacks) {
		callback();
	}
}

/**
 * Mock implementation of OrbisModule.
 * Replace with actual WASM module when C++ embind bindings are ready.
 */
export const mockModule: OrbisModule = {
	getOverlayState(): OverlayState {
		return { ...overlayState };
	},

	setOverlay(overlay: keyof OverlayState, enabled: boolean): void {
		overlayState[overlay] = enabled;
		notifyStateChange();
	},

	setTimeScale(_scale: number): void {
		notifyStateChange();
	},

	initCanvas(_canvas: HTMLCanvasElement): void {
		console.log('[Mock] Canvas initialized');
	},

	resize(_width: number, _height: number): void {
		// Mock: would resize render target
	}
};

/**
 * Subscribe to state changes.
 * Returns unsubscribe function.
 */
export function onStateChange(callback: StateChangeCallback): () => void {
	stateChangeCallbacks.push(callback);
	return () => {
		const index = stateChangeCallbacks.indexOf(callback);
		if (index >= 0) {
			stateChangeCallbacks.splice(index, 1);
		}
	};
}

/**
 * Call the C++ SetGroupFilterEnabled binding via emscripten.
 */
export function callSetGroupFilterEnabled(groupId: string, enabled: boolean): void {
	const wasmModule = (window as Record<string, unknown>).Module as
		| { setGroupFilterEnabled?: (groupId: string, enabled: boolean) => void }
		| undefined;
	if (wasmModule?.setGroupFilterEnabled) {
		wasmModule.setGroupFilterEnabled(groupId, enabled);
	}
}

/**
 * Get the current module instance.
 * In production, this would load and return the actual WASM module.
 */
export function getModule(): OrbisModule {
	return mockModule;
}

/**
 * Load the WASM module asynchronously.
 * For now, returns the mock module immediately.
 */
export async function loadModule(): Promise<OrbisModule> {
	// TODO: Replace with actual WASM loading:
	// const module = await import('/game.js');
	// return await module.default();
	return mockModule;
}
