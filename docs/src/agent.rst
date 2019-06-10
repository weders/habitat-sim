Agent
=====

.. currentmodule:: habitat_sim

Agent
-----



.. autoclass:: Agent
   :members:
   :undoc-members:

.. autoclass:: AgentConfiguration
   :members:
   :undoc-members:


.. autoclass:: AgentState
   :members:
   :undoc-members:

.. autoclass:: SixDOFPose
   :members:
   :undoc-members:



Actions
-------

.. autoclass:: ActionSpec
   :members:
   :undoc-members:


.. autoclass:: ActuationSpec
   :members:
   :undoc-members:

.. autoclass:: habitat_sim.agent.controls.SceneNodeControl
   :members:
   :undoc-members:


.. autodecorator:: habitat_sim.agent.controls.register_move_fn


We currently have the following actions added by default.  Any action
not registered with an explict name is given the snake case version of the class name,
i.e. ``MoveForward`` can be accessed with the name `move_forward`

.. literalinclude:: ../../habitat_sim/agent/default_controls.py



Action space path finding
-------------------------

.. autoclass:: habitat_sim.GreedyGeodesicFollower
   :members:
