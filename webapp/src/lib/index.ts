// Components
export { default as Canvas } from './components/Canvas.svelte';
export { default as InfoPanel } from './components/InfoPanel.svelte';
export { default as FilterControls } from './components/FilterControls.svelte';
export { default as OverlayManager } from './components/OverlayManager.svelte';

// Stores
export * from './stores/game';

// Types
export type * from './wasm/types';

// WASM module
export { getModule, loadModule, onStateChange } from './wasm/module';
