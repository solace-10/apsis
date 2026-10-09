<script lang="ts">
	import { selectedObject, selectObject } from '$lib/stores/game';

	function formatNumber(value: number, decimals: number = 2): string {
		return value.toFixed(decimals);
	}

	function getObjectTypeLabel(type: string): string {
		const labels: Record<string, string> = {
			satellite: 'Satellite',
			debris: 'Debris',
			rocket_body: 'Rocket Body',
			space_station: 'Space Station',
			unknown: 'Unknown'
		};
		return labels[type] || type;
	}

	function getOrbitType(altitude: number): string {
		if (altitude < 2000) return 'LEO';
		if (altitude < 35786) return 'MEO';
		if (altitude >= 35786 && altitude <= 35800) return 'GEO';
		return 'HEO';
	}

	function handleClose() {
		selectObject(null);
	}

	// Keeps the name on one line: condense the width first (down to 80%), then shrink the size.
	// Text width scales roughly linearly with both, so each step is a single measurement.
	function fitOneLine(node: HTMLElement, name: string) {
		let fittedName = '';

		async function fit(name: string) {
			// The store re-emits on every position update, so only refit when the name changes.
			if (name === fittedName) return;
			fittedName = name;
			node.style.fontStretch = '';
			node.style.fontSize = '';
			await document.fonts.ready;

			let ratio = node.clientWidth / node.scrollWidth;
			if (ratio >= 1) return;
			node.style.fontStretch = `${Math.max(ratio, 0.8) * 100}%`;

			ratio = node.clientWidth / node.scrollWidth;
			if (ratio < 1) {
				node.style.fontSize = `${Math.max(20, Math.floor(parseFloat(getComputedStyle(node).fontSize) * ratio))}px`;
			}
		}

		fit(name);
		return { update: fit };
	}
</script>

{#if $selectedObject}
	<div class="info-panel panel">
		<header class="panel-header">
			<span class="panel-title">Selected object</span>
			<span class="badge">{getOrbitType($selectedObject.altitude)}</span>
			<button class="icon-btn" onclick={handleClose} aria-label="Close selected object">
				<svg class="icon" viewBox="0 0 16 16" aria-hidden="true"><path d="M3.5 3.5l9 9M12.5 3.5l-9 9" /></svg>
			</button>
		</header>

		<div class="sat-header">
			<div class="sat-name" use:fitOneLine={$selectedObject.name}>{$selectedObject.name}</div>
			<div class="data-label">NORAD {$selectedObject.noradId}</div>
		</div>

		<div class="data-grid">
			<div class="data-item">
				<span class="data-label">Intl. designator</span>
				<span class="data-value">{$selectedObject.internationalDesignator}</span>
			</div>
			<div class="data-item">
				<span class="data-label">Type</span>
				<span class="data-value">{getObjectTypeLabel($selectedObject.objectType)}</span>
			</div>
			<div class="data-item">
				<span class="data-label">Altitude</span>
				<span class="data-value">{formatNumber($selectedObject.altitude, 1)}<span class="data-unit">km</span></span>
			</div>
			<div class="data-item">
				<span class="data-label">Velocity</span>
				<span class="data-value">{Math.round($selectedObject.velocity * 3600).toLocaleString('en-US')}<span class="data-unit">km/h</span></span>
			</div>
			<div class="data-item">
				<span class="data-label">Latitude</span>
				<span class="data-value">{formatNumber($selectedObject.latitude, 2)}°</span>
			</div>
			<div class="data-item">
				<span class="data-label">Longitude</span>
				<span class="data-value">{formatNumber($selectedObject.longitude, 2)}°</span>
			</div>
		</div>

		<!-- Disclosures sharing this name are exclusive: opening one closes the others. -->
		<details class="disclosure" name="info-panel">
			<summary>
				<span class="disclosure-title">Orbit mean elements</span>
				<svg class="icon chevron" viewBox="0 0 16 16" aria-hidden="true"><path d="M4 6l4 4 4-4" /></svg>
			</summary>
			<div class="data-grid">
				<div class="data-item">
					<span class="data-label">Eccentricity</span>
					<span class="data-value">{formatNumber($selectedObject.eccentricity, 6)}</span>
				</div>
				<div class="data-item">
					<span class="data-label">Inclination</span>
					<span class="data-value">{formatNumber($selectedObject.inclination, 2)}°</span>
				</div>
				<div class="data-item">
					<span class="data-label">RAAN</span>
					<span class="data-value">{formatNumber($selectedObject.raan, 2)}°</span>
				</div>
				<div class="data-item">
					<span class="data-label">Arg. perigee</span>
					<span class="data-value">{formatNumber($selectedObject.argOfPerigee, 2)}°</span>
				</div>
				<div class="data-item">
					<span class="data-label">Mean anomaly</span>
					<span class="data-value">{formatNumber($selectedObject.meanAnomaly, 2)}°</span>
				</div>
			</div>
		</details>
	</div>
{/if}

<style>
	.info-panel {
		position: fixed;
		top: 16px;
		right: 16px;
		width: 360px;
		max-height: calc(100vh - 32px);
		overflow-y: auto;
	}

	.sat-header {
		display: flex;
		flex-direction: column;
		gap: 8px;
		padding: 16px 14px 14px;
	}

	.sat-name {
		font-size: 44px;
		line-height: 44px;
		font-weight: 800;
		letter-spacing: -0.01em;
		white-space: nowrap;
		overflow: hidden;
		text-overflow: ellipsis;
	}

	.disclosure {
		border-top: 1px solid var(--line);
	}

	.disclosure summary {
		display: flex;
		align-items: center;
		gap: 10px;
		min-height: 44px;
		padding: 0 14px;
		list-style: none;
		cursor: pointer;
	}

	.disclosure summary::-webkit-details-marker {
		display: none;
	}

	.disclosure summary:hover {
		background: var(--surface-raised);
	}

	.disclosure-title {
		flex: 1;
		font-size: 11px;
		line-height: 16px;
		font-weight: 800;
		letter-spacing: 0.16em;
		text-transform: uppercase;
	}

	.disclosure[open] .chevron {
		transform: rotate(180deg);
	}

	.disclosure .data-grid {
		border-top: none;
		padding-top: 0;
	}
</style>
