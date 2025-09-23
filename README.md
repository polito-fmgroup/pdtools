# pdtools
A portfolio-based suite for hardware model checking.

#### Generating an invariant with IC3 and checking it against the original model
The following command let us generate a baseline invariant as a result of model checking a given input model, provided in `.aig` format, using the using the *IC3* engine within `pdtrav`.
```
pdtrav -strategy ic3 [-verbosity 4] -writeInvar <output_invar_filename> <input_model_filename>
```
The following command let us certify the generated invariant against the original model using the `pdtapp` utility.
```
pdtapp certify <input_model_filename> <input_invar_filename>
```

#### Phase Abstraction
If phase abstraction (pa) were to be applied to the original input model, thus producing an intermediate model, verification happens on the transformed/simplified model, whereas certification has to take into account the original model.
```
pdtrav -strategy ic3 [-verbosity 4] -writeInvar <output_invar_filename> <input_pa_model_filename>
```
In order to support certification, it is also necessary to provide the number of phases identified during phase abstraction, as as `-f <F>`
```
pdtapp certify -f <F> <input_model_filename> <input_invar_filename>
```

#### Temporal Decomposition
In case of temporal decomposition (td), an additional parameter `-peripheralLatches` has to be included in the verification command
```
pdtrav -strategy ic3 [-verbosity 4] -writeInvar <output_invar_filename> -peripheralLatches <input_model_filename>
```
and the `<output_invar_filename>` must end in `-T#.aig` to encode the number of steps considered for temporal decomposition itself.
Such a value must be then provided as a parameter during certification, as `-t <T>`.

```
pdtapp certify -t <T> <input_model_filename> <input_invar_filename>
```

#### Rewriting models with ABC 
It can be convenient to include an explicit symbol table, in order to uniquely identify all the components of a given model, regardless of the sequence of transformation applied. To do so, one can exploit ABC rewriting capabilities, as:
```
abc -c "read <filein>;write <fileout>;"
```

It can be convenient to preprocess models in bulk and store their transformed counterparts after phase abstraction has been applied. To do so, one can exploit ABC rewriting capabilities, as:
```
abc -c "read <filein>;frames -F <F>;orpos;write {fileout};"
```