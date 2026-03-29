/* Cabal - Legacy Game Implementations
 *
 * Engine plugins table for embedded build.
 * This replaces the auto-generated file from the configure system.
 */

#ifndef ENGINES_PLUGINS_TABLE_H
#define ENGINES_PLUGINS_TABLE_H

// Engine plugin declarations
#ifdef ENABLE_AGI
LINK_PLUGIN(AGI)
#endif

#ifdef ENABLE_GOB
LINK_PLUGIN(GOB)
#endif

#ifdef ENABLE_KYRA
LINK_PLUGIN(KYRA)
#endif

#ifdef ENABLE_SCUMM
LINK_PLUGIN(SCUMM)
#endif

#endif // ENGINES_PLUGINS_TABLE_H
