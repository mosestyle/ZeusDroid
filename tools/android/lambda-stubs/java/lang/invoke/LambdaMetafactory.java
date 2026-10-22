/*
 * Compile-time-only stub so javac can emit invokedynamic lambdas against the
 * Android framework jar (which omits these methods). d8 desugars all lambdas,
 * so nothing here is referenced at runtime and the class is never packaged.
 * Written for this project; equivalent in purpose to the SDK's
 * core-lambda-stubs.jar.
 */
package java.lang.invoke;

public final class LambdaMetafactory {
    public static final int FLAG_SERIALIZABLE = 1;
    public static final int FLAG_MARKERS = 2;
    public static final int FLAG_BRIDGES = 4;

    private LambdaMetafactory() {}

    public static CallSite metafactory(MethodHandles.Lookup caller, String invokedName,
                                       MethodType invokedType, MethodType samMethodType,
                                       MethodHandle implMethod, MethodType instantiatedMethodType)
            throws LambdaConversionException {
        throw new UnsupportedOperationException("compile-time stub");
    }

    public static CallSite altMetafactory(MethodHandles.Lookup caller, String invokedName,
                                          MethodType invokedType, Object... args)
            throws LambdaConversionException {
        throw new UnsupportedOperationException("compile-time stub");
    }
}
