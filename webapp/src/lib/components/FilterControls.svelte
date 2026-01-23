<script lang="ts">
	import { groups, toggleGroupVisibility, setGroupVisibility } from '$lib/stores/game';

	let expanded = true;

	function formatCount(count: number): string {
		if (count >= 1000) {
			return `${(count / 1000).toFixed(1)}k`;
		}
		return count.toString();
	}

	function handleToggleAll(visible: boolean) {
		for (const group of $groups) {
			setGroupVisibility(group.id, visible);
		}
	}
</script>

<div class="filter-panel panel" class:collapsed={!expanded}>
	<header class="panel-header">
		<button class="toggle-btn" onclick={() => (expanded = !expanded)} aria-label="Toggle panel">
			<span class="toggle-icon">{expanded ? '◀' : '▶'}</span>
		</button>
		{#if expanded}
			<span class="panel-title">Object Groups</span>
			<div class="header-actions">
				<button class="btn btn-sm" onclick={() => handleToggleAll(true)} title="Show all">
					All
				</button>
				<button class="btn btn-sm" onclick={() => handleToggleAll(false)} title="Hide all">
					None
				</button>
			</div>
		{/if}
	</header>

	{#if expanded}
		<div class="panel-body panel-content">
			{#each $groups as group (group.id)}
				<label class="group-item">
					<input
						type="checkbox"
						checked={group.visible}
						onchange={() => toggleGroupVisibility(group.id)}
					/>
					<span class="group-color" style="--color: {group.color}"></span>
					<span class="group-name">{group.name}</span>
					<span class="group-count">{formatCount(group.count)}</span>
				</label>
			{/each}
		</div>
	{/if}
</div>

<style>
	.filter-panel {
		position: fixed;
		top: 1rem;
		left: 1rem;
		width: 260px;
		transition: width 0.2s ease;
	}

	.filter-panel.collapsed {
		width: auto;
	}

	.filter-panel.collapsed .panel-header {
		border-bottom: none;
		padding: 0.75rem;
	}

	.toggle-btn {
		background: none;
		border: none;
		color: var(--text-dim);
		cursor: pointer;
		padding: 0.25rem;
		display: flex;
		align-items: center;
		justify-content: center;
		transition: color 0.15s ease;
	}

	.toggle-btn:hover {
		color: var(--text-primary);
	}

	.toggle-icon {
		font-size: 10px;
	}

	.header-actions {
		display: flex;
		gap: 0.5rem;
	}

	.btn-sm {
		font-family: var(--font-mono);
		font-size: 0.6875rem;
		font-weight: 500;
		text-transform: uppercase;
		letter-spacing: 0.03em;
		padding: 0.25rem 0.5rem;
		border: 1px solid var(--border-default);
		border-radius: 3px;
		background: transparent;
		color: var(--text-secondary);
		cursor: pointer;
		transition: all 0.15s ease;
	}

	.btn-sm:hover {
		background: var(--bg-hover);
		color: var(--text-primary);
		border-color: var(--text-dim);
	}

	.panel-content {
		padding: 0.5rem;
		max-height: calc(100vh - 8rem);
		overflow-y: auto;
	}

	.group-item {
		display: flex;
		align-items: center;
		gap: 0.75rem;
		padding: 0.5rem;
		border-radius: 3px;
		cursor: pointer;
		transition: background 0.15s ease;
	}

	.group-item:hover {
		background: var(--bg-hover);
	}

	.group-item input[type='checkbox'] {
		width: 16px;
		height: 16px;
		accent-color: var(--accent-primary);
		cursor: pointer;
	}

	.group-color {
		width: 12px;
		height: 12px;
		border-radius: 3px;
		background: var(--color);
		flex-shrink: 0;
	}

	.group-name {
		flex: 1;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
		font-family: var(--font-mono);
		font-size: 0.8125rem;
	}

	.group-count {
		font-family: var(--font-mono);
		color: var(--text-dim);
		font-size: 0.75rem;
		font-variant-numeric: tabular-nums;
	}
</style>
