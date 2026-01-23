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
</script>

{#if $selectedObject}
	<div class="info-panel panel">
		<header class="panel-header">
			<span class="panel-title">Selected Object</span>
			<div class="header-right">
				<span class="badge badge-primary">{getOrbitType($selectedObject.altitude)}</span>
				<button class="close-btn" onclick={handleClose} aria-label="Close panel">×</button>
			</div>
		</header>

		<div class="panel-body">
			<div class="sat-header">
				<div>
					<div class="sat-name">{$selectedObject.name}</div>
					<div class="sat-norad">NORAD {$selectedObject.noradId}</div>
				</div>
				<div class="sat-status">
					<span class="status-dot active"></span>
					<span class="status-text">Active</span>
				</div>
			</div>

			<div class="data-grid">
				<div class="data-item">
					<span class="data-label">Altitude</span>
					<span class="data-value highlight">{formatNumber($selectedObject.altitude, 1)}<span class="data-unit">km</span></span>
				</div>
				<div class="data-item">
					<span class="data-label">Velocity</span>
					<span class="data-value">{formatNumber($selectedObject.velocity, 2)}<span class="data-unit">km/s</span></span>
				</div>
				<div class="data-item">
					<span class="data-label">Inclination</span>
					<span class="data-value">{formatNumber($selectedObject.inclination, 2)}°</span>
				</div>
				<div class="data-item">
					<span class="data-label">Type</span>
					<span class="data-value">{getObjectTypeLabel($selectedObject.objectType)}</span>
				</div>
			</div>

			<div class="orbital-section">
				<div class="section-title">Orbital Elements</div>
				<div class="orbital-grid">
					<div class="orbital-item">
						<div class="orbital-label">Semi-Major</div>
						<div class="orbital-value">{formatNumber($selectedObject.semiMajorAxis, 1)} km</div>
					</div>
					<div class="orbital-item">
						<div class="orbital-label">Eccentricity</div>
						<div class="orbital-value">{formatNumber($selectedObject.eccentricity, 6)}</div>
					</div>
					<div class="orbital-item">
						<div class="orbital-label">Inclination</div>
						<div class="orbital-value">{formatNumber($selectedObject.inclination, 2)}°</div>
					</div>
					<div class="orbital-item">
						<div class="orbital-label">RAAN</div>
						<div class="orbital-value">{formatNumber($selectedObject.raan, 2)}°</div>
					</div>
					<div class="orbital-item">
						<div class="orbital-label">Arg. Perigee</div>
						<div class="orbital-value">{formatNumber($selectedObject.argOfPerigee, 2)}°</div>
					</div>
					<div class="orbital-item">
						<div class="orbital-label">Mean Anomaly</div>
						<div class="orbital-value">{formatNumber($selectedObject.meanAnomaly, 2)}°</div>
					</div>
				</div>
			</div>

			<div class="btn-group">
				<button class="btn btn-primary">Track</button>
				<button class="btn">Predict</button>
				<button class="btn">Details</button>
			</div>
		</div>
	</div>
{/if}

<style>
	.info-panel {
		position: fixed;
		top: 1rem;
		right: 1rem;
		width: 340px;
		max-height: calc(100vh - 2rem);
		overflow-y: auto;
	}

	.header-right {
		display: flex;
		align-items: center;
		gap: 0.75rem;
	}

	.close-btn {
		background: none;
		border: none;
		color: var(--text-dim);
		font-size: 20px;
		cursor: pointer;
		padding: 0;
		line-height: 1;
		transition: color 0.15s ease;
	}

	.close-btn:hover {
		color: var(--text-primary);
	}

	.sat-header {
		display: flex;
		align-items: flex-start;
		justify-content: space-between;
		margin-bottom: 1.25rem;
	}

	.sat-name {
		font-family: var(--font-mono);
		font-size: 1.125rem;
		font-weight: 600;
		color: var(--text-primary);
		margin-bottom: 0.25rem;
	}

	.sat-norad {
		font-family: var(--font-mono);
		font-size: 0.75rem;
		color: var(--text-dim);
	}

	.sat-status {
		display: flex;
		align-items: center;
		gap: 0.5rem;
		font-family: var(--font-mono);
		font-size: 0.6875rem;
		text-transform: uppercase;
		letter-spacing: 0.05em;
	}

	.status-dot {
		width: 8px;
		height: 8px;
		border-radius: 50%;
		animation: pulse 2s ease-in-out infinite;
	}

	.status-dot.active {
		background: var(--accent-success);
	}

	.status-text {
		color: var(--accent-success);
	}

	@keyframes pulse {
		0%, 100% { opacity: 1; }
		50% { opacity: 0.5; }
	}

	.orbital-section {
		margin-top: 1.25rem;
		padding-top: 1.25rem;
		border-top: 1px solid var(--border-subtle);
	}

	.section-title {
		font-family: var(--font-mono);
		font-size: 0.6875rem;
		text-transform: uppercase;
		letter-spacing: 0.05em;
		color: var(--text-dim);
		margin-bottom: 0.75rem;
	}

	.btn-group {
		display: flex;
		gap: 0.5rem;
		margin-top: 1.25rem;
		padding-top: 1.25rem;
		border-top: 1px solid var(--border-subtle);
	}
</style>
