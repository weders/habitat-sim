.. Habitat Sim Documentation masterfile

Habitat Simulator
=================

A flexible, high-performance 3D simulator with configurable agents, multiple sensors, and generic 3D dataset handling
(with built-in support for MatterPort3D, Gibson, Replica, and other datasets).
When rendering a scene from the Matterport3D dataset, Habitat-Sim achieves several thousand frames per second (FPS) running single-threaded,
and reaches over 10,000 FPS multi-process on a single GPU!

.. warning::
   Docs are currently very much a work-in-progress and are incomplete :(

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   src/simulator.rst

   src/agent.rst

   src/utils.rst

   src/backend.rst

.. toctree::
   :maxdepth: 1
   :caption: Examples

   examples/new_actions.rst

   examples/stereo_agent.rst

   examples/notebooks.rst
