"""Read-only verification of the saved PSX navigation."""
import unreal

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/ChopIt/World/Maps/L_PSX_test')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
bounds = [a for a in actors if isinstance(a, unreal.NavMeshBoundsVolume)]
assert bounds
for actor in bounds:
    center, extent = actor.get_actor_bounds(False)
    assert min(extent.x, extent.y, extent.z) > 1, str(extent)
nav = unreal.NavigationSystemV1.get_navigation_system(world)
assert nav
# Commandlets hold an editor async-load build lock; query the serialized data directly.
point = unreal.NavigationSystemV1.get_random_reachable_point_in_radius(world, unreal.Vector(0, 0, 0), 1500)
assert point is not None, 'No reachable point in saved navigation'
unreal.log('PSX_NAV_VERIFIED saved bounds valid; reachable point: {}'.format(point))
