/**
 * TypeScript interfaces for Orbis game bindings.
 * These will be implemented via C++ embind later.
 */

/**
 * Raw interop data from C++ WebInterop callbacks.
 * Field names match the C++ SpaceObjectInterop struct.
 */
export interface SpaceObjectInterop {
	objectName: string;
	objectId: string;
	noradCatalogueId: number;
	eccentricity: number;
	inclination: number;
	rightAscensionOfAscendingNode: number;
	argumentOfPericenter: number;
	meanAnomaly: number;
	semiMajorAxis: number;
	altitude: number;
	velocity: number;
	latitude: number;
	longitude: number;
}

/**
 * Convert raw interop data to SpaceObject.
 */
export function fromInterop(interop: SpaceObjectInterop): SpaceObject {
	return {
		id: interop.noradCatalogueId,
		name: interop.objectName,
		noradId: interop.noradCatalogueId,
		objectType: 'satellite', // TODO: Add object type to interop
		group: { id: 'unknown', name: 'Unknown', color: '#888888', visible: true, count: 0 },
		semiMajorAxis: interop.semiMajorAxis,
		eccentricity: interop.eccentricity,
		inclination: interop.inclination,
		raan: interop.rightAscensionOfAscendingNode,
		argOfPerigee: interop.argumentOfPericenter,
		meanAnomaly: interop.meanAnomaly,
		altitude: interop.altitude,
		velocity: interop.velocity,
		latitude: interop.latitude,
		longitude: interop.longitude
	};
}

export interface SpaceObject {
	id: number;
	name: string;
	noradId: number;
	objectType: SpaceObjectType;
	group: SpaceObjectGroup;

	// Orbital parameters
	semiMajorAxis: number;      // km
	eccentricity: number;
	inclination: number;        // degrees
	raan: number;               // Right Ascension of Ascending Node (degrees)
	argOfPerigee: number;       // degrees
	meanAnomaly: number;        // degrees

	// Current state
	altitude: number;           // km above Earth surface
	velocity: number;           // km/s
	latitude: number;           // degrees
	longitude: number;          // degrees
}

export type SpaceObjectType =
	| 'satellite'
	| 'debris'
	| 'rocket_body'
	| 'space_station'
	| 'unknown';

export interface SpaceObjectGroup {
	id: string;
	name: string;
	color: string;
	visible: boolean;
	count: number;
}

export interface OverlayState {
	grid: boolean;
	atmosphere: boolean;
	labels: boolean;
	orbits: boolean;
	groundTracks: boolean;
}

export interface GameState {
	selectedObject: SpaceObject | null;
	groups: SpaceObjectGroup[];
	overlays: OverlayState;
	objectCount: number;
	simulationTime: Date;
	timeScale: number;
}

/**
 * Interface for the WASM module bindings.
 * Will be implemented by embind in C++.
 */
export interface OrbisModule {
	// Query functions
	getSpaceObjects(): SpaceObject[];
	getSpaceObjectById(id: number): SpaceObject | null;
	getGroups(): SpaceObjectGroup[];
	getSelectedObject(): SpaceObject | null;
	getOverlayState(): OverlayState;

	// Mutation functions
	selectObject(id: number | null): void;
	setGroupVisibility(groupId: string, visible: boolean): void;
	setOverlay(overlay: keyof OverlayState, enabled: boolean): void;
	setTimeScale(scale: number): void;

	// Canvas management
	initCanvas(canvas: HTMLCanvasElement): void;
	resize(width: number, height: number): void;
}
