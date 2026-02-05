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

// Callbacks registered by the app
let onSelectionChange: SpaceObjectCallback | null = null;
let onObjectUpdate: SpaceObjectCallback | null = null;

/**
 * Register callbacks for space object updates from C++.
 */
export function registerCallbacks(
	onSelected: SpaceObjectCallback,
	onUpdated: SpaceObjectCallback
): void {
	onSelectionChange = onSelected;
	onObjectUpdate = onUpdated;
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
		}
	};

	(window as unknown as { orbisCallbacks: OrbisCallbacks }).orbisCallbacks = callbacks;
}

// Mock data for development
const mockGroups: SpaceObjectGroup[] = [
	{ id: 'last30days', name: 'Last 30 days\' launches', color: '#CC6600', visible: true, count: 5 },
	{ id: 'stations', name: 'Space Stations', color: '#FFD700', visible: true, count: 5 },
	{ id: 'starlink', name: 'Starlink', color: '#4A90D9', visible: true, count: 5400 },
	{ id: 'oneweb', name: 'OneWeb', color: '#7B68EE', visible: true, count: 634 },
	{ id: 'gps', name: 'GPS', color: '#32CD32', visible: true, count: 31 },
	{ id: 'gnss', name: 'GNSS', color: '#FF6347', visible: true, count: 24 },
	{ id: 'geo', name: 'Active geosynchronous', color: '#00CED1', visible: true, count: 28 },
	{ id: 'science', name: 'Science', color: '#87CEEB', visible: true, count: 42 },
	{ id: 'cosmos-1408-debris', name: 'Russian ASAT test debris', color: '#880040', visible: false, count: 23000 },
	{ id: 'debris', name: 'Debris', color: '#808080', visible: false, count: 23000 },
	{ id: 'analyst', name: 'Well-tracked analyst', color: '#404040', visible: false, count: 200 },
	{ id: 'other', name: 'Other', color: '#DD8080', visible: false, count: 200 }
];

const mockObjects: SpaceObject[] = [
	{
		id: 1,
		name: 'ISS (ZARYA)',
		noradId: 25544,
		objectType: 'space_station',
		group: mockGroups[0],
		semiMajorAxis: 6798,
		eccentricity: 0.0001,
		inclination: 51.64,
		raan: 247.5,
		argOfPerigee: 130.5,
		meanAnomaly: 45.2,
		altitude: 420,
		velocity: 7.66,
		latitude: 32.5,
		longitude: -95.2
	},
	{
		id: 2,
		name: 'TIANGONG',
		noradId: 48274,
		objectType: 'space_station',
		group: mockGroups[0],
		semiMajorAxis: 6780,
		eccentricity: 0.0002,
		inclination: 41.47,
		raan: 180.2,
		argOfPerigee: 95.3,
		meanAnomaly: 120.8,
		altitude: 390,
		velocity: 7.68,
		latitude: -15.3,
		longitude: 45.7
	},
	{
		id: 3,
		name: 'STARLINK-1234',
		noradId: 44238,
		objectType: 'satellite',
		group: mockGroups[1],
		semiMajorAxis: 6921,
		eccentricity: 0.0001,
		inclination: 53.0,
		raan: 120.3,
		argOfPerigee: 90.0,
		meanAnomaly: 270.5,
		altitude: 550,
		velocity: 7.59,
		latitude: 45.2,
		longitude: -120.8
	},
	{
		id: 4,
		name: 'NAVSTAR 78 (USA 304)',
		noradId: 48859,
		objectType: 'satellite',
		group: mockGroups[3],
		semiMajorAxis: 26560,
		eccentricity: 0.01,
		inclination: 55.0,
		raan: 60.0,
		argOfPerigee: 45.0,
		meanAnomaly: 180.0,
		altitude: 20200,
		velocity: 3.87,
		latitude: 22.1,
		longitude: 78.4
	},
	{
		id: 5,
		name: 'COSMOS 2251 DEB',
		noradId: 34427,
		objectType: 'debris',
		group: mockGroups[7],
		semiMajorAxis: 7150,
		eccentricity: 0.02,
		inclination: 74.0,
		raan: 300.0,
		argOfPerigee: 200.0,
		meanAnomaly: 90.0,
		altitude: 780,
		velocity: 7.45,
		latitude: 68.5,
		longitude: -30.2
	}
];

let selectedObjectId: number | null = null;
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
	getSpaceObjects(): SpaceObject[] {
		return mockObjects;
	},

	getSpaceObjectById(id: number): SpaceObject | null {
		return mockObjects.find((obj) => obj.id === id) ?? null;
	},

	getGroups(): SpaceObjectGroup[] {
		return mockGroups;
	},

	getSelectedObject(): SpaceObject | null {
		if (selectedObjectId === null) return null;
		return mockObjects.find((obj) => obj.id === selectedObjectId) ?? null;
	},

	getOverlayState(): OverlayState {
		return { ...overlayState };
	},

	selectObject(id: number | null): void {
		selectedObjectId = id;
		notifyStateChange();
	},

	setGroupVisibility(groupId: string, visible: boolean): void {
		const group = mockGroups.find((g) => g.id === groupId);
		if (group) {
			group.visible = visible;
			notifyStateChange();
		}
	},

	setOverlay(overlay: keyof OverlayState, enabled: boolean): void {
		overlayState[overlay] = enabled;
		notifyStateChange();
	},

	setTimeScale(_scale: number): void {
		// Mock: does nothing
		notifyStateChange();
	},

	initCanvas(_canvas: HTMLCanvasElement): void {
		// Mock: would initialize WebGPU context
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
